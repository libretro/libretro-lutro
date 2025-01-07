#include "audio.h"
#include "lutro.h"
#include "compat/strl.h"
#include "lutro_assert.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <file/file_path.h>
#include <audio/conversion/float_to_s16.h>
#include <math.h>
#include <errno.h>

/* TODO/FIXME - no sound on big-endian */

// any source which is playing must maintain a ref in lua, to avoid __gc.
typedef struct {
   int           lua_ref;
} audioSourceByRef;


static int num_sources = 0;
static audioSourceByRef* sources_playing = NULL;
static float volume = 1.0;

#define CHANNELS 2

static int16_t saturate(mixer_presaturate_t in) {
   if (in >=  INT16_MAX) { return INT16_MAX; }
   if (in <=  INT16_MIN) { return INT16_MIN; }
   return cvt_presaturate_to_int16(in);
}

audio_Source* getSourcePtrFromRef(lua_State* L, audioSourceByRef ref)
{
   if (ref.lua_ref < 0)
      return NULL;

   lua_getglobal(L, "refs_audio_playing");
   lua_rawgeti(L, -1, ref.lua_ref);
   audio_Source* result = lua_touserdata(L, -1);
   lua_pop(L, 2);
   return result;
}

bool sourceIsPlayable(audio_Source* source)
{
   return source && (source->wavData || source->oggData || source->sndta);
}

// unrefs stopped sounds.
// this is done periodically by lutro to avoid adding lua dependencies to the mixer.
void mixer_unref_stopped_sounds(lua_State* L)
{
   // Loop over audio sources
   for (int i = 0; i < num_sources; i++)
   {
      if (sources_playing[i].lua_ref >= 0)
      {
         audio_Source* source = getSourcePtrFromRef(L, sources_playing[i]);
         if (!source || source->state == AUDIO_STOPPED)
         {
            // only unref is source is non-NULL -- assume a null source is already unref'd and that the
            // stale part of our data is just sources_playing (to avoid unref'ing something that might already
            // be re-using this ref ID)

            if (source)
            {
               lua_getglobal(L, "refs_audio_playing");
               luaL_unref(L, -1, sources_playing[i].lua_ref);
               lua_pop(L,1);
            }
            sources_playing[i].lua_ref = LUA_REFNIL;
         }
      }
   }
}

void lutro_audio_stop_all(lua_State* L)
{
   // Loop over audio sources
   // no cleanup needed, __gc will handle it later after a call to mixer_unref_stopped_sounds()
   for (int i = 0; i < num_sources; i++)
   {
      audio_Source* source = getSourcePtrFromRef(L, sources_playing[i]);
      if (source)
         source->state = AUDIO_STOPPED;
   }
}

#if LUTRO_BUILD_IS_TOOL
#  define mixer_buffer_guardband 64
#else
#  define mixer_buffer_guardband 0
#endif

typedef struct mixer_presaturate_t_guarded
{
#if mixer_buffer_guardband
   uint8_t guard_f[mixer_buffer_guardband];
#endif

   mixer_presaturate_t presaturated[(AUDIO_FRAMES * CHANNELS)];

#if mixer_buffer_guardband
   uint8_t guard_b[mixer_buffer_guardband];
#endif
} mixer_presaturate_t_guarded;

void mixer_render(lua_State* L, int16_t *buffer)
{
   static mixer_presaturate_t_guarded localbuffer;

   memset(localbuffer.presaturated, 0, sizeof(localbuffer.presaturated));

#if mixer_buffer_guardband
   memset(localbuffer.guard_f, 0xcd, sizeof(localbuffer.guard_f));
   memset(localbuffer.guard_b, 0xcd, sizeof(localbuffer.guard_f));
#endif

   presaturate_buffer_desc bufdesc;
   bufdesc.data      = localbuffer.presaturated;
   bufdesc.channels  = CHANNELS;
   bufdesc.samplelen = AUDIO_FRAMES;

   // Loop over audio sources
   for (int i = 0; i < num_sources; i++)
   {
      audio_Source* source = getSourcePtrFromRef(L, sources_playing[i]);

      if (!source)
         continue;

      if (source->state == AUDIO_STOPPED)
         continue;

      if (source->state == AUDIO_PAUSED)
         continue;

      // options here are to premultiply source volumes with master volume, or apply master volume at the end of mixing
      // during the saturation step. Each approach has its strengths and weaknesses and overall neither differs much when
      // using float or double for presaturation buffer (see final saturation step below)
      float srcvol = source->volume;

      // Decoder Seek Position Note:
      //   It's unclear if a decoder can be shared by multiple sources, so always seek the decoder for each chunk.
      //   Our decoder APIs internally optimize away redundant seeks.

      if (source->oggData)
      {
         decOgg_seek(source->oggData, source->sndpos);
         bool finished = decOgg_decode(source->oggData, &bufdesc, srcvol, source->loop);
         if (finished)
         {
            decOgg_seek(source->oggData, 0);    // see notes above
            source->state  = AUDIO_STOPPED;
            source->sndpos = 0;
         }
         source->sndpos = decOgg_sampleTell(source->oggData);
         continue;
      }

      if (source->wavData)
      {
         decWav_seek(source->wavData, source->sndpos);    // see notes above
         bool finished = decWav_decode(source->wavData, &bufdesc, srcvol, source->loop);
         if (finished)
         {
            decWav_seek(source->wavData, 0);
            source->state  = AUDIO_STOPPED;
         }
         source->sndpos = decWav_sampleTell(source->wavData);
         continue;
      }

      if (source->sndta)
      {
         snd_SoundData* sndta = source->sndta;

         int total_mixed = 0;

         while (total_mixed < AUDIO_FRAMES)
         {
            int mixchunksz = AUDIO_FRAMES - total_mixed;
            int remaining  = sndta->numSamples - source->sndpos;
            if (mixchunksz > remaining)
            {
               mixchunksz = remaining;
            }
            if (sndta->numChannels == 1)
            {
               for (int j = 0; j < mixchunksz; ++j, ++total_mixed, ++source->sndpos)
               {
                  localbuffer.presaturated[(total_mixed*2) + 0] += sndta->data[source->sndpos] * srcvol;
                  localbuffer.presaturated[(total_mixed*2) + 1] += sndta->data[source->sndpos] * srcvol;
               }
            }

            if (sndta->numChannels == 2)
            {
               for (int j = 0; j < mixchunksz; ++j, ++total_mixed, ++source->sndpos)
               {
                  localbuffer.presaturated[(total_mixed*2) + 0] += sndta->data[(source->sndpos*2) + 0] * srcvol;
                  localbuffer.presaturated[(total_mixed*2) + 1] += sndta->data[(source->sndpos*2) + 1] * srcvol;
               }
            }

            dbg_assume(source->sndpos <= sndta->numSamples);
            dbg_assume(total_mixed <= AUDIO_FRAMES);

            if (source->sndpos == sndta->numSamples)
            {
               source->sndpos = 0;

               if (!source->loop)
               {
                  source->state = AUDIO_STOPPED;
                  break;
               }
            }
         }
         continue;
      }

      // invalid source object - possibly asset loading failed, was invalid or it's been partially GC'd.
      source->state = AUDIO_STOPPED;
   }

   // final saturation step - downsample.
   float mastervol_and_scale_to_int16 = volume * 32767;
   for (int j = 0; j < AUDIO_FRAMES * CHANNELS; j++)
   {
      buffer[j] = saturate(localbuffer.presaturated[j] * mastervol_and_scale_to_int16);
   }

#if mixer_buffer_guardband
   if (mixer_buffer_guardband > 0) {
      int bi;
      for (bi=0; bi < mixer_buffer_guardband; ++bi)
      {
         tool_assert(localbuffer.guard_f[bi] == 0xcd);
         tool_assert(localbuffer.guard_b[bi] == 0xcd);
      }
   }
#endif
}

int lutro_audio_preload(lua_State *L)
{
   static const luaL_Reg audio_funcs[] =  {
      { "play",      audio_play },
      { "stop",      audio_stop },
      { "pause",     audio_pause },
      { "newSource", audio_newSource },
      { "getVolume", audio_getVolume },
      { "setVolume", audio_setVolume },
      { "getActiveSources",      audio_getActiveSources },
      { "getActiveSourceCount",  audio_getActiveSourceCount },
      {NULL, NULL}
   };

   lutro_newlib(L, audio_funcs, "audio");

   return 1;
}

void lutro_audio_init(lua_State* L)
{
   num_sources = 0;
   sources_playing = NULL;
   volume = 1.0;

   lua_newtable(L);
   lua_setglobal(L, "refs_audio_playing");
}

void lutro_audio_deinit()
{
   if (!sources_playing) return;

   // lua owns most of our objects so there's only two proper ways to deinit audio:
   //  1. assume luaState has been forcibly destroyed without its own cleanup.
   //  2. run lua_close() and let it clean most of this up first.

   int counted = 0;
   for (int i = 0; i < num_sources; i++)
   {
      if (sources_playing[i].lua_ref >= 0)
         ++counted;
   }

   if (counted)
   {
      fprintf(stderr, "Found %d leaked audio source references. Was lua_close() called first?\n", counted);
      //assert(false);
      return;
   }

   lutro_free(sources_playing);
   sources_playing = NULL;
   num_sources = 0;
}

static int find_empty_source_slot(audio_Source* self)
{
   for(int i=0; i<num_sources; ++i)
   {
      // it's possible a stopped source is still in the sources_playing list, since cleanup
      // operations are deferred. This is OK and expected, just use the slot that's still assigned...

      if (sources_playing[i].lua_ref < 0)
         return i;
   }
   return -1;
}

// value in the stack specified by 'idx' should be the userdata(audio_Source), either created using newuserdata,
// or grabbed from the appropriate registry table.
// returns reference to the lua table 'Source'
static void make_metatable_Source(lua_State* L, int stidx_udata)
{
   if (luaL_newmetatable(L, "Source") != 0)
   {
      static const luaL_Reg audio_funcs[] = {
         { "play",       audio_play }, /* We can reuse audio_play and */
         { "stop",       audio_stop }, /* audio_stop here. */
         { "setLooping", source_setLooping },
         { "isLooping",  source_isLooping },
         { "isStopped",  source_isStopped },
         { "pause",      source_pause },
         { "isPaused",   source_isPaused },
         { "isPlaying",  source_isPlaying },
         { "setVolume",  source_setVolume },
         { "getVolume",  source_getVolume },
         { "seek",       source_seek },
         { "tell",       source_tell },
         { "setPitch",   source_setPitch },
         { "getPitch",   source_getPitch },
         { "__gc",       source_gc },
         {NULL, NULL}
      };

      lua_pushvalue(L, -1);
      lua_setfield(L, -2, "__index");

      lua_pushcfunction(L, source_gc);
      lua_setfield( L, -2, "__gc" );

      luaL_setfuncs(L, audio_funcs, 0);
   }

   lua_setmetatable(L, stidx_udata);
}

int audio_newSource(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 1 && n != 2)
      return luaL_error(L, "lutro.audio.newSource requires 1 or 2 arguments, %d given.", n);

   audio_Source* self = (audio_Source*)lua_newuserdata(L, sizeof(audio_Source));
   int stidx_udata = lua_gettop(L);
   self->oggData = NULL;
   self->wavData = NULL;
   self->sndta   = NULL;
   self->lua_ref_sndta = LUA_REFNIL;

   void *p = lua_touserdata(L, 1);
   if (p == NULL)
   {
      // when matching file paths, only lua string types are OK.
      // lua_tostring will convert numbers into strings implicitly, which is not what we want.
      luaL_checktype(L, 1, LUA_TSTRING);     // explicit non-converted string type test

      const char* path = lua_tostring(L, 1);
      AssetPathInfo asset;
      lutro_assetPath_init(&asset, path);

      if (strstr(asset.ext, "ogg"))
      {
         self->oggData = lutro_malloc(sizeof(dec_OggData));
         if (!decOgg_init(self->oggData, asset.fullpath))
         {
            lutro_free(self->oggData);
            self->oggData = NULL;
         }
      }

      if (strstr(asset.ext, "wav"))
      {
         self->wavData = lutro_malloc(sizeof(dec_WavData));
         if (!decWav_init(self->wavData, asset.fullpath))
         {
            lutro_free(self->wavData);
            self->wavData = NULL;
         }
      }

   }
   else
   {
      snd_SoundData* sndta = (snd_SoundData*)luaL_checkudata(L, 1, "SoundData");
      self->sndta = sndta;

      lua_pushvalue(L, 1);    // push ref to SoundData parameter
      self->lua_ref_sndta = luaL_ref(L, LUA_REGISTRYINDEX);
   }

   self->loop = false;
   self->volume = 1.0;
   self->sndpos = 0;
   self->state = AUDIO_STOPPED;

   make_metatable_Source(L, stidx_udata);
   return 1;
}

int audio_setVolume(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 1)
      return luaL_error(L, "lutro.audio.setVolume requires 1 argument, %d given.", n);

   volume = (float)luaL_checknumber(L, 1);

   return 0;
}

int audio_getVolume(lua_State *L)
{
   lua_pushnumber(L, volume);
   return 1;
}

int source_setLooping(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 2)
      return luaL_error(L, "Source:setLooping requires 2 arguments, %d given.", n);

   audio_Source* self = (audio_Source*)luaL_checkudata(L, 1, "Source");
   bool loop = lua_toboolean(L, 2);
   self->loop = loop;

   return 0;
}

int source_pause(lua_State *L)
{
   audio_Source* self = (audio_Source*)luaL_checkudata(L, 1, "Source");
   if (self->state != AUDIO_STOPPED)
      self->state = AUDIO_PAUSED;
   return 1;
}

int source_isLooping(lua_State *L)
{
   audio_Source* self = (audio_Source*)luaL_checkudata(L, 1, "Source");
   lua_pushboolean(L, self->loop);
   return 1;
}

int source_isStopped(lua_State *L)
{
   audio_Source* self = (audio_Source*)luaL_checkudata(L, 1, "Source");
   lua_pushboolean(L, (self->state == AUDIO_STOPPED));
   return 1;
}

int source_isPaused(lua_State *L)
{
   audio_Source* self = (audio_Source*)luaL_checkudata(L, 1, "Source");
   lua_pushboolean(L, (self->state == AUDIO_PAUSED));
   return 1;
}

int source_isPlaying(lua_State *L)
{
   audio_Source* self = (audio_Source*)luaL_checkudata(L, 1, "Source");
   lua_pushboolean(L, (self->state == AUDIO_PLAYING));
   return 1;
}

int source_setVolume(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 2)
      return luaL_error(L, "Source:setVolume requires 2 arguments, %d given.", n);

   audio_Source* self = (audio_Source*)luaL_checkudata(L, 1, "Source");
   self->volume = (float)luaL_checknumber(L, 2);

   return 0;
}

int source_getVolume(lua_State *L)
{
   audio_Source* self = (audio_Source*)luaL_checkudata(L, 1, "Source");
   lua_pushnumber(L, self->volume);
   return 1;
}

int source_tell(lua_State *L)
{
   audio_Source* self = (audio_Source*)luaL_checkudata(L, 1, "Source");

   const char* type = lua_isstring(L,2) ? lua_tostring(L,2) : NULL;

   // sndpos should always be accurate for any given source or stream.
   // (validation of this would be best performed per-frame, before and/or after mixing, and not here)

   intmax_t npSamples = self->sndpos;

   if (type)
   {
      if (strcmp(type, "seconds") == 0)
      {
         double npSeconds = npSamples / 44100.0;
         lua_pushnumber(L, npSeconds);
      }
      else if(strcmp(type, "samples") == 0)
      {
         lua_pushinteger(L, npSamples);
      }
      else
      {
         return luaL_error(L, "Source:tell '%s' given for second argument. Expected either 'seconds' or 'samples'", type);
      }
   }
   else
   {
      lua_pushinteger(L, npSamples);
   }
   return 1;
}

int source_seek(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 3)
      return luaL_error(L, "Source:seek requires 3 arguments, %d given.", n);

   audio_Source* self = (audio_Source*)luaL_checkudata(L, 1, "Source");

   const char* type = lua_isstring(L,3) ? lua_tostring(L, 3) : NULL;

   intmax_t npSamples = 0;

   if (type)
   {
      if (strcmp(type, "seconds") == 0)
      {
         double npSeconds = luaL_checknumber(L, 2);
         npSamples = npSeconds * 44100.0;
      }
      else if(strcmp(type, "samples") == 0)
      {
         npSamples = luaL_checkinteger(L, 2);
      }
      else
      {
         return luaL_error(L, "Source:seek '%s' given for third argument. Expected either 'seconds' or 'samples'", type);
      }
   }
   else
   {
      // assume samples if third paramter is unspecified
      npSamples = luaL_checkinteger(L, 2);
   }

   if (self->wavData)
   {
      if (!decWav_seek(self->wavData, npSamples))
      {
         // TODO: it'd be nice to log with the full lua FILE(LINE): context that's normally prefixed by lua_error,
         // in a manner that allows us to log it without stopping the system. --jstine
         fprintf(stderr, "WAV decoder seek failed: %s\n", strerror(errno));
      }
      self->sndpos = decWav_sampleTell(self->wavData);
   }
   else if (self->oggData)
   {
      if (!decOgg_seek(self->oggData, npSamples))
      {
         fprintf(stderr, "OGG decoder seek failed: %s\n", strerror(errno));
      }
      self->sndpos = decOgg_sampleTell(self->oggData);
   }

   if (self->sndta)
   {
      self->sndpos = npSamples;
      if (self->sndpos > self->sndta->numSamples)
         self->sndpos = self->sndta->numSamples;
   }

   if (npSamples != self->sndpos)
   {
      // the underlying media source will fixup the seek position...
      fprintf(stderr, "warning: seek asked for sample pos %jd, got pos %jd\n", npSamples, self->sndpos);
   }
   return 0;
}

int source_setPitch(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 2)
      return luaL_error(L, "Source:setPitch requires 2 arguments, %d given.", n);

   audio_Source* self = (audio_Source*)luaL_checkudata(L, 1, "Source");
   self->pitch = (float)luaL_checknumber(L, 2);

   return 0;
}

int source_getPitch(lua_State *L)
{
   audio_Source* self = (audio_Source*)luaL_checkudata(L, 1, "Source");
   lua_pushnumber(L, self->pitch);
   return 1;
}

int source_gc(lua_State *L)
{
   audio_Source* self = (audio_Source*)luaL_checkudata(L, 1, "Source");

   luaL_unref(L, LUA_REGISTRYINDEX, self->lua_ref_sndta);
   self->lua_ref_sndta = LUA_REFNIL;

   if (self->wavData)
   {
      if (self->wavData->fp)
         fclose(self->wavData->fp);

      lutro_free(self->wavData);
   }

   if (self->oggData)
   {
      ov_clear(&self->oggData->vf);
      lutro_free(self->oggData);
   }

   (void)self;
   return 0;
}


// audio.play returns nothing, but source:play returns a boolean.
// it's OK enough for us to always return a boolean.
int audio_play(lua_State *L)
{
   audio_Source* self = (audio_Source*)luaL_checkudata(L, 1, "Source");

   //play pos should not reset if play called again before finished
   //as in Love2D, game code should explicitly call stop or seek(0, nil) before play to reset pos if already playing

   if (self->state == AUDIO_PLAYING)
      return 0;     // nothing to do.

   if (!sourceIsPlayable(self))
   {
      lutro_errorf("Audio source is not playable.", self->state);
      self->state = AUDIO_STOPPED;
      return 0;
   }

   if (self->state == AUDIO_PAUSED)
   {
      self->state = AUDIO_PLAYING;
      return 0;
   }

   if (self->state != AUDIO_STOPPED)
   {
      lutro_alertf("Invalid audio state value=%d", self->state);
      return 0;
   }

   self->state = AUDIO_PLAYING;

   // add a ref to our playing audio registry. this blocks __gc until the ref(s) are removed.

   int slot = find_empty_source_slot(self);
   if (slot < 0)
   {
      // TODO: This should be converted into a pooled allocator (a linked list of reusable blocks)

      slot = num_sources++;
      audioSourceByRef *new_sources_playing = (audioSourceByRef*)lutro_realloc(sources_playing, num_sources * sizeof(audioSourceByRef));
      if (new_sources_playing == NULL)
         lutro_alertf("Not enough memory reallocating sources_playing");
      else
         sources_playing = new_sources_playing;

      sources_playing[slot].lua_ref = LUA_REFNIL;
   }

   // assume that find_empty_source_slot with either return the current slot, or an empty one.
   if (sources_playing[slot].lua_ref < 0)
   {
      lua_getglobal(L, "refs_audio_playing");
      lua_pushvalue(L, 1);    // push ref to Source parameter
      sources_playing[slot].lua_ref = luaL_ref(L, -2);
   }
   else
   {
      // existing ref means it should be our same source already.
      // (technically this should be unreachable, and we might want to assert if this is ever
      //  reached regardless if it matches or not).
      dbg_assert(getSourcePtrFromRef(L, sources_playing[slot]) == self);
   }

   // for now sources always succeed in lutro.
   // the only reason for a source to fail in Love2D is because it has a limited number of mixer
   // voices internally that it allows. Lutro has no hard limit on mixer voices.

   lua_pushboolean(L, 1);
   return 1;
}

// returns nothing.
int audio_stop(lua_State *L)
{
   audio_Source* self = (audio_Source*)luaL_checkudata(L, 1, "Source");

   if (self->state == AUDIO_STOPPED)
      return 0;

   self->sndpos = 0;
   self->state  = AUDIO_STOPPED;

   // unref will be handled via periodic invocation of mixer_unref_stopped_sounds()
   //lua_getglobal(L, "refs_audio_playing");
   //luaL_unref(L, -1, self->lua_ref);
   //self->lua_ref = LUA_REFNIL;

   return 0;
}

enum {
   FILTER_PLAYING          = (1<<AUDIO_PLAYING),
   FILTER_PAUSED           = (1<<AUDIO_PAUSED),
   FILTER_STOPPED          = (1<<AUDIO_STOPPED),
   FILTER_ACTIVE           = (FILTER_PLAYING | FILTER_PAUSED),
   FILTER_RESULT_TABLE     = (1<<8),
   FILTER_RESULT_COUNT     = (0),
   FILTER_ALL              = (0xff)
};

static void getActiveSourcesFiltered(lua_State *L, int sourceStateFilterMask)
{
   int table_insert_idx = 1;     // could also use luaL_getn(L, index_of_table)
   int stidx_tbl_result = 0;
   if (sourceStateFilterMask & FILTER_RESULT_TABLE)
   {
      lua_newtable(L);
      stidx_tbl_result = lua_gettop(L);
   }

   lua_getglobal(L, "refs_audio_playing");

   for (int i = 0; i < num_sources; i++)
   {
      lua_pushinteger(L, sources_playing[i].lua_ref);
      lua_gettable(L, -2);
      audio_Source* source = lua_touserdata(L, -1);

      if (!source) {
         lua_pop(L, 1);
         continue;
      }

      if ((sourceStateFilterMask & (1<<source->state)) == 0) {
         lua_pop(L, 1);
         continue;
      }

      if (sourceStateFilterMask & FILTER_RESULT_TABLE)
         lua_rawseti(L, stidx_tbl_result, table_insert_idx);  // pops result
      else
         lua_pop(L, 1);

      ++table_insert_idx;
   }

   lua_pop(L, 1);  // refs_audio_playing

   if (sourceStateFilterMask & FILTER_RESULT_TABLE)
   {
      // do nothing, table is already on the stack at -1
   }
   else
   {
      lua_pushinteger(L, table_insert_idx-1);
   }
}

int audio_getActiveSources(lua_State *L)
{
   getActiveSourcesFiltered(L, FILTER_ACTIVE | FILTER_RESULT_TABLE);
   return 1;
}

int audio_getActiveSourceCount(lua_State *L)
{
   getActiveSourcesFiltered(L, FILTER_ACTIVE | FILTER_RESULT_COUNT);
   return 1;
}


static void pause_sources_in_table(lua_State* L, int idx)
{
}

// returns list of sources paused by this call.
// love2D docs indicate that it only returns a value when called with no arguments. This hardly
// makes sense - might as well return a list of sources which were paused regardless. The user
// may optionally pass in a list of sources and this will filter them and return just the ones
// that were not already paused.
int audio_pause(lua_State *L)
{
   int nargs = lua_gettop(L);

   lua_newtable(L);

   if (nargs == 0)
   {
      getActiveSourcesFiltered(L, FILTER_PLAYING | FILTER_RESULT_TABLE);
      lua_pushnil(L);  /* first key */
      while (lua_next(L, -2) != 0)
      {
         // uses 'key' (at index -2) and 'value' (at index -1)
         audio_Source* self = (audio_Source*)luaL_checkudata(L, -1, "Source");
         dbg_assume(self);    // some kind of table management problem

         self->state = AUDIO_PAUSED;
         lua_pop(L, 1);       // remove 'value'; keep 'key' for next iteration
      }
   }
   else
   {
      int table_insert_idx = 1;
      int i;
      for(i=1; i<nargs+1; ++i)
      {
         int type = lua_type(L, i);
         if (type == LUA_TTABLE)
         {
            audio_Source* self = (audio_Source*)luaL_checkudata(L, -1, "Source");
            if (self)
            {
               self->state = AUDIO_PAUSED;
               lua_rawseti(L, -3, table_insert_idx);
               ++table_insert_idx;
            }
            else
            {
               lua_pushnil(L);  /* first key */
               while (lua_next(L, i) != 0)
               {
                  // uses 'key' (at index -2) and 'value' (at index -1)
                  self = (audio_Source*)luaL_checkudata(L, -1, "Source");
                  if (self)
                  {
                     self->state = AUDIO_PAUSED;
                     lua_rawseti(L, nargs+1, table_insert_idx);
                     ++table_insert_idx;
                  }
                  lua_pop(L, 1);       // remove 'value'; keep 'key' for next iteration
               }
            }
         }
      }
   }

   return 1;

#if 0
   }

   if (lua_istable(L, 1))

   audio_Source* self = (audio_Source*)luaL_checkudata(L, 1, "Source");

   if (self->state == AUDIO_STOPPED)
      return 0;

   self->sndpos = 0;
   self->state  = AUDIO_STOPPED;

#endif

   return 0;
}
