#include "audio.h"
#include "lutro.h"
#include "compat/strl.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <file/file_path.h>
#include <audio/conversion/float_to_s16.h>
#include <math.h>
#include <assert.h>

/* TODO/FIXME - no sound on big-endian */

// any source which is playing must maintain a ref in lua, to avoid __gc.
typedef struct {
   audio_Source* source;
   int           lua_ref;
} audioSourceWithRef;


static int num_sources = 0;
static audioSourceWithRef* sources_playing = NULL;
static float volume = 1.0;

#define CHANNELS 2

static int16_t saturate(mixer_presaturate_t in) {
   if (in >=  INT16_MAX) { return INT16_MAX; }
   if (in <=  INT16_MIN) { return INT16_MIN; }
   return cvt_presaturate_to_int16(in);
}

int audio_sources_nullify_refs(const audio_Source* source)
{
   int counted = 0;
   if (!source) return 0;

   // rather than crash, let's nullify any known references here,
   // even if they're currently playing (they'll be cut to silence)

   for(int i=0; i<num_sources; ++i)
   {
      audioSourceWithRef* srcref = &sources_playing[i];
      if (srcref->source == source)
      {
         if (srcref->source->state != AUDIO_STOPPED)
            ++counted;
   
         // do not free - the pointers in sources are lua user data
         srcref->source = NULL;
         srcref->lua_ref = LUA_NOREF;
      }
   }

   return counted;
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
         audio_Source* source = sources_playing[i].source;
         if (!source || source->state == AUDIO_STOPPED)
         {
            lua_getglobal(L, "refs_audio_playing");
            luaL_unref(L, -1, sources_playing[i].lua_ref);
            sources_playing[i].lua_ref = LUA_REFNIL;
         }
      }
      if (sources_playing[i].lua_ref < 0)
         sources_playing[i].source  = NULL;
   }
}

void lutro_audio_stop_all(void)
{
   // Loop over audio sources
   // no cleanup needed, __gc will handle it later after a call to mixer_unref_stopped_sounds()
   for (int i = 0; i < num_sources; i++)
   {
      if (sources_playing[i].source)
         sources_playing[i].source->state = AUDIO_STOPPED;
   }
}

void mixer_render(int16_t *buffer)
{
   static mixer_presaturate_t presaturateBuffer[AUDIO_FRAMES * CHANNELS];

   memset(presaturateBuffer, 0, AUDIO_FRAMES * CHANNELS * sizeof(mixer_presaturate_t));

   presaturate_buffer_desc bufdesc;
   bufdesc.data      = presaturateBuffer;
   bufdesc.channels  = CHANNELS;
   bufdesc.samplelen = AUDIO_FRAMES;

   // Loop over audio sources
   for (int i = 0; i < num_sources; i++)
   {
      audio_Source* source = sources_playing[i].source;
      if (!source)
         continue;

      if (source->state == AUDIO_STOPPED)
         continue;

      // options here are to premultiply source volumes with master volume, or apply master volume at the end of mixing
      // during the saturation step. Each approach has its strengths and weaknesses and overall neither differs much when
      // using float or double for presaturation buffer (see final saturation step below)
      float srcvol = source->volume;

      if (source->oggData)
      {
         bool finished = decoder_decodeOgg(source->oggData, &bufdesc, srcvol, source->loop);
         if (finished)
         {
            decoder_seek(source->oggData, 0);
            source->state  = AUDIO_STOPPED;
            source->sndpos = 0;
         }
         source->sndpos = decoder_sampleTell(source->oggData);
         continue;
      }

      if (source->wavData)
      {
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
                  presaturateBuffer[(total_mixed*2) + 0] += sndta->data[source->sndpos] * srcvol;
                  presaturateBuffer[(total_mixed*2) + 1] += sndta->data[source->sndpos] * srcvol;
               }  
            }

            if (sndta->numChannels == 2)
            {
               for (int j = 0; j < mixchunksz; ++j, ++total_mixed, ++source->sndpos)
               {
                  presaturateBuffer[(total_mixed*2) + 0] += sndta->data[(source->sndpos*2) + 0] * srcvol;
                  presaturateBuffer[(total_mixed*2) + 1] += sndta->data[(source->sndpos*2) + 1] * srcvol;
               }  
            }

            assert(source->sndpos <= sndta->numSamples);
            assert(total_mixed <= AUDIO_FRAMES);

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
      }
   }

   // final saturation step - downsample.
   float mastervol_and_scale_to_int16 = volume * 32767;
   for (int j = 0; j < AUDIO_FRAMES * CHANNELS; j++)
   {
      buffer[j] = saturate(presaturateBuffer[j] * mastervol_and_scale_to_int16);
   }
}

int lutro_audio_preload(lua_State *L)
{
   static luaL_Reg gfx_funcs[] =  {
      { "play",      audio_play },
      { "stop",      audio_stop },
      { "newSource", audio_newSource },
      { "getVolume", audio_getVolume },
      { "setVolume", audio_setVolume },
      {NULL, NULL}
   };

   lutro_ensure_global_table(L, "lutro");

   luaL_newlib(L, gfx_funcs);

   lua_setfield(L, -2, "audio");

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

   lutro_audio_stop_all();
   int counted = 0;
   for (int i = 0; i < num_sources; i++)
   {
      if (sources_playing[i].source)
         ++counted;
   }

   if (counted)
   {
      fprintf(stderr, "Found %d leaked audio source references. Was lua_close() called first?\n", counted);
      fflush(stderr);
      assert(false);
      return;
   }

   free(sources_playing);
   sources_playing = NULL;
   num_sources = 0;
}

static int find_empty_source_slot(audio_Source* self)
{
   for(int i=0; i<num_sources; ++i)
   {
      // it's possible a stopped source is still in the sources_playing list, since cleanup
      // operations are deferred. This is OK and expected, just use the slot that's still assigned...

      if (!sources_playing[i].source || sources_playing[i].source == self)
         return i;
   }
   return -1;
}

int audio_newSource(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 1 && n != 2)
      return luaL_error(L, "lutro.audio.newSource requires 1 or 2 arguments, %d given.", n);

   audio_Source* self = (audio_Source*)lua_newuserdata(L, sizeof(audio_Source));
   self->oggData = NULL;
   self->wavData = NULL;

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
         self->oggData = malloc(sizeof(dec_OggData));
         decoder_initOgg(self->oggData, asset.fullpath);
      }

      if (strstr(asset.ext, "wav"))
      {
         self->wavData = malloc(sizeof(dec_WavData));
         decWav_init(self->wavData, asset.fullpath);
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

   if (luaL_newmetatable(L, "Source") != 0)
   {
      static luaL_Reg audio_funcs[] = {
         { "play",       audio_play }, /* We can reuse audio_play and */
         { "stop",       audio_stop }, /* audio_stop here. */
         { "setLooping", source_setLooping },
         { "isLooping",  source_isLooping },
         { "isStopped",  source_isStopped },
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

   lua_setmetatable(L, -2);

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

   //currently assuming samples vs seconds
   //TODO: check if 2nd param is "seconds" or "samples"

   // sndpos should always be accurate for any given source or stream.
   lua_pushnumber(L, self->sndpos);

   return 1;
}

int source_seek(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 3)
      return luaL_error(L, "Source:seek requires 3 arguments, %d given.", n);

   audio_Source* self = (audio_Source*)luaL_checkudata(L, 1, "Source");
 
   //currently assuming samples vs seconds
   //TODO: check if 3rd param is "seconds" or "samples"
 
   self->sndpos = luaL_checkinteger(L, 2);

   if (self->wavData)
      decWav_seek(self->wavData, self->sndpos);
   
   if (self->oggData)
      decoder_seek(self->oggData, self->sndpos);

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

   // todo - add some info to help identify the offending leaker.
   // (don't get carried away tho - this message is only really useful to lutro core devs since
   //  it indiciates a failure of our internal Lua/C glue)

   int leaks = audio_sources_nullify_refs(self);
   if (leaks)
   {
      fprintf(stderr, "source_gc: playing audio references were nullified.\n");
      fflush(stderr);
      //assert(false);
   }
   
   luaL_unref(L, LUA_REGISTRYINDEX, self->lua_ref_sndta);
   self->lua_ref_sndta = LUA_REFNIL;

   if (self->wavData)
   {
      if (self->wavData->fp)
         fclose(self->wavData->fp);

      free(self->wavData);
   }

   if (self->oggData)
   {
      ov_clear(&self->oggData->vf);
      free(self->oggData);
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

   if (self->state == AUDIO_PAUSED)
   {
      self->state = AUDIO_PLAYING;
      return 0;
   }

   assert(self->state == AUDIO_STOPPED);
   self->state = AUDIO_PLAYING;

   // add a ref to our playing audio registry. this blocks __gc until the ref(s) are removed.

   int slot = find_empty_source_slot(self);
   if (slot < 0)
   {
      slot = num_sources++;
      sources_playing = (audioSourceWithRef*)realloc(sources_playing, num_sources * sizeof(audioSourceWithRef));
      sources_playing[slot].lua_ref = LUA_REFNIL;
      sources_playing[slot].source  = NULL;
   }

   // assume that find_empty_source_slot with either return the current slot, or an empty one.
   if (sources_playing[slot].lua_ref < 0)
   {
      lua_getglobal(L, "refs_audio_playing");
      lua_pushvalue(L, 1);    // push ref to Source parameter
      sources_playing[slot].source  = self;
      sources_playing[slot].lua_ref = luaL_ref(L, -2);
   }
   else
   {
      // existing ref means it should be our same source already.
      assert(sources_playing[slot].source == self);
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
