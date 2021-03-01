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

static unsigned num_sources = 0;
static audio_Source** sources = NULL;
static float volume = 1.0;

#define CHANNELS 2

static int16_t saturate(mixer_presaturate_t in) {
   if (in >=  INT16_MAX) { return INT16_MAX; }
   if (in <=  INT16_MIN) { return INT16_MIN; }
   return cvt_presaturate_to_int16(in);
}

int audio_sources_nullify_refs(const snd_SoundData* sndta)
{
   int counted = 0;
   if (!sndta->fp) return 0;

   // rather than crash, let's nullify any known references here,
   // even if they're currently playing (they'll be cut to silence)

   for(int i=0; i<num_sources; ++i)
   {
      if (sources[i] && sources[i]->sndta.fp == sndta->fp)
      {
         if (sources[i]->state != AUDIO_STOPPED)
            ++counted;
   
         sources[i] = NULL;
      }
   }

   return counted;
}

void mixer_render(int16_t *buffer)
{
   static mixer_presaturate_t presaturateBuffer[AUDIO_FRAMES * CHANNELS];

   memset(presaturateBuffer, 0, AUDIO_FRAMES * CHANNELS * sizeof(mixer_presaturate_t));

   // Loop over audio sources
   for (int i = 0; i < num_sources; i++)
   {
      if (!sources[i])
         continue;

      if (sources[i]->state == AUDIO_STOPPED)
         continue;

      // options here are to premultiply source volumes with master volume, or apply master volume at the end of mixing
      // during the saturation step. Each approach has its strengths and weaknesses and overall neither differs much when
      // using float or double for presaturation buffer (see final saturation step below)
      float srcvol = sources[i]->volume;

      if (sources[i]->oggData)
      {
         bool finished = decoder_decodeOgg(sources[i]->oggData, presaturateBuffer, srcvol, sources[i]->loop);
         if (finished)
         {
            decoder_seek(sources[i]->oggData, 0);
            sources[i]->state = AUDIO_STOPPED;
         }
         continue;
      }
      
      void* rawsamples_alloc = calloc(
         AUDIO_FRAMES * sources[i]->bps, sizeof(uint8_t));

      fseek(sources[i]->sndta.fp, WAV_HEADER_SIZE + sources[i]->pos, SEEK_SET);

      bool end = ! fread(rawsamples_alloc,
            sizeof(uint8_t),
            AUDIO_FRAMES * sources[i]->bps,
            sources[i]->sndta.fp);

      // ogg outputs float values range 1.0 to -1.0
      // 16-bit wav outputs values range 32767 to -32768
      // 8-bit wav is scaled up to 16 bit and then normalized using 16-bit divisor.
      float srcvol_and_scale_to_one = srcvol / 32767;

      if (sources[i]->sndta.head.BitsPerSample ==  8)
      {
         const int8_t* rawsamples8 = (int8_t*)rawsamples_alloc;
         for (int j = 0; j < AUDIO_FRAMES; j++)
         {
            // note this is currently *64 because the mixer is mixing 8-bit smaples as 0->255 instead of normalizing to -128 to 127.
            // This would have caused the more appropriate *128 multiplier to cause saturation along the top of the waveform.
            // We should be able to change this to *128 now, but it will affect volume behavior of any games that use 8 bit samples and
            // were authored with the current *64 behavior. So need to verify how we want to go about handling this --jstine

            mixer_presaturate_t left  = (sources[i]->sndta.head.NumChannels == 2) ? rawsamples8[j*2+0] : rawsamples8[j] * 64;
            mixer_presaturate_t right = (sources[i]->sndta.head.NumChannels == 2) ? rawsamples8[j*2+1] : rawsamples8[j] * 64;

            if (sources[i]->sndta.head.NumChannels == 2) { right = rawsamples8[j*2+1]*64; }

            presaturateBuffer[j*2+0] += (left  * srcvol_and_scale_to_one);
            presaturateBuffer[j*2+1] += (right * srcvol_and_scale_to_one);
            sources[i]->pos += sources[i]->bps;
         }
      }

      if (sources[i]->sndta.head.BitsPerSample ==  16)
      {
         const int16_t* rawsamples16 = (int16_t*)rawsamples_alloc;
         for (int j = 0; j < AUDIO_FRAMES; j++)
         {
            mixer_presaturate_t left  = (sources[i]->sndta.head.NumChannels == 2) ? rawsamples16[j*2+0] : rawsamples16[j];
            mixer_presaturate_t right = (sources[i]->sndta.head.NumChannels == 2) ? rawsamples16[j*2+1] : rawsamples16[j];

            if (sources[i]->sndta.head.NumChannels == 2) { right = rawsamples16[j*2+1]; }

            presaturateBuffer[j*2+0] += (left  * srcvol_and_scale_to_one);
            presaturateBuffer[j*2+1] += (right * srcvol_and_scale_to_one);
            sources[i]->pos += sources[i]->bps;
         }
      }

      if (end)
      {
         if (!sources[i]->loop)
            sources[i]->state = AUDIO_STOPPED;
         sources[i]->pos = 0;
      }

      free(rawsamples_alloc);
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

void lutro_audio_init()
{
   num_sources = 0;
   sources = NULL;
   volume = 1.0;
}

void lutro_audio_deinit()
{
   if (sources)
   {
      for (unsigned i = 0; i < num_sources; i++)
      {
         if (sources[i] && sources[i]->oggData)
         {
            ov_clear(&sources[i]->oggData->vf);
            free(sources[i]->oggData);
         }
      }

      free(sources);
      sources = NULL;
      num_sources = 0;
   }
}

static int assign_to_existing_source_slot(audio_Source* self)
{
   for(int i=0; i<num_sources; ++i)
   {
      if (!sources[i])
      {
         sources[i] = self;
         return 1;
      }
   }
   return 0;
}

int audio_newSource(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 1 && n != 2)
      return luaL_error(L, "lutro.audio.newSource requires 1 or 2 arguments, %d given.", n);

   audio_Source* self = (audio_Source*)lua_newuserdata(L, sizeof(audio_Source));
   self->oggData = NULL;
   self->sndta.fp = NULL;        

   //if (lua_isstring(L,1)) // (lua_type(L, 1) == LUA_TSTRING)
   //{
   //   
   //}

   void *p = lua_touserdata(L, 1);
   if (p == NULL)
   {
      const char* path = luaL_checkstring(L, 1);

      char fullpath[PATH_MAX_LENGTH];
      strlcpy(fullpath, settings.gamedir, sizeof(fullpath));
      strlcat(fullpath, path, sizeof(fullpath));

      //get file extension
      char ext[PATH_MAX_LENGTH];
      strcpy(ext, path_get_extension(path));
      for(int i = 0; ext[i]; i++)
         ext[i] = tolower((uint8_t)ext[i]);
      
      //ogg
      if (strstr(ext, "ogg"))
      {
         self->oggData = malloc(sizeof(OggData));
         decoder_initOgg(self->oggData, fullpath);
      }
      //default: WAV file
      else
      {
         FILE *fp = fopen(fullpath, "rb");
         if (!fp)
            return -1;

         fread(&self->sndta.head, sizeof(uint8_t), WAV_HEADER_SIZE, fp);
         self->sndta.fp = fp;
      }
   }
   else
   {
      snd_SoundData* sndta = (snd_SoundData*)luaL_checkudata(L, 1, "SoundData");
      self->sndta = *sndta;
   }

   self->loop = false;
   self->volume = 1.0;
   self->pos = 0;
   self->state = AUDIO_STOPPED;

   //WAV file
   if (self->sndta.fp)
   {
      self->bps = self->sndta.head.NumChannels * self->sndta.head.BitsPerSample / 8;
      fseek(self->sndta.fp, 0, SEEK_END);
   }

   if (!assign_to_existing_source_slot(self))
   {
      num_sources++;
      sources = (audio_Source**)realloc(sources, num_sources * sizeof(audio_Source));
      sources[num_sources-1] = self;
   }

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

   //WAV file
   if (self->sndta.fp)
   {
      lua_pushnumber(L, self->pos / self->bps);
   }
   //OGG file
   else if (self->oggData)
   {
      uint32_t pos = 0;
      decoder_sampleTell(self->oggData, &pos);
      lua_pushnumber(L, pos);
   }

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
 
   //WAV file
   if (self->sndta.fp)
   {
      self->pos = self->bps * (unsigned)luaL_checknumber(L, 2);
   }
   //OGG file
   else if (self->oggData)
   {
      decoder_seek(self->oggData, (unsigned)luaL_checknumber(L, 2));
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

   // todo - add some info to help identify the offending leaker.
   // (don't get carried away tho - this message is only really useful to lutro core devs since
   //  it indiciates a failure of our internal Lua/C glue)

   int leaks = audio_sources_nullify_refs(self);
   if (leaks)
   {
      fprintf(stderr, "source_gc: playing audio references were nullified.\n");
      //assert(false);
   }

   (void)self;
   return 0;
}

int audio_play(lua_State *L)
{
   audio_Source* self = (audio_Source*)luaL_checkudata(L, 1, "Source");

   //play pos should not reset if play called again before finished
   //as in Love2D, game code should explicitly call stop or seek(0, nil) before play to reset pos if already playing
   self->state = AUDIO_PLAYING;
   
   lua_pushboolean(L, true);
   return 1;
}

int audio_stop(lua_State *L)
{
   audio_Source* self = (audio_Source*)luaL_checkudata(L, 1, "Source");
   bool success = false;

   //play pos is reset on stop

   //WAV file
   if (self->sndta.fp)
   {
      self->pos = 0;
      self->state = AUDIO_STOPPED;
   }
   //OGG file
   else if (self->oggData)
   {
      success = decoder_seek(self->oggData, 0);
      self->state = AUDIO_STOPPED;
   }
   
   lua_pushboolean(L, success);
   return 1;
}
