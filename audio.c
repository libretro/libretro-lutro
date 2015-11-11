#include "audio.h"
#include "lutro.h"
#include "compat/strl.h"
#include "rthreads/rthreads.h"

#include <stdlib.h>
#include <string.h>

/* TODO/FIXME - no sound on big-endian */

static audio_Source** sources           = NULL;
static size_t         sources_count     = 0;
static size_t         sources_allocated = 0;
static slock_t*       sources_lock      = NULL;

static float volume = 1.0;

static void add_source(audio_Source *src)
{
   slock_lock(sources_lock);

   if (sources_count == sources_allocated)
   {
      size_t new_size = (sources_allocated + 1) * 2;
      sources = (audio_Source**)realloc(sources, new_size * sizeof(audio_Source*));
      memset(&sources[sources_allocated], 0, (new_size - sources_allocated) * sizeof(audio_Source*));

      sources_allocated = new_size;
   }

   sources[sources_count++] = src;

   slock_unlock(sources_lock);
}

static void remove_source(const audio_Source *src)
{
   unsigned i;

   slock_lock(sources_lock);

   for (i = 0; i < sources_count; ++i)
   {
      if (sources[i] == src)
      {
         memmove(&sources[i], sources[i+1], (sources_count-i-1) * sizeof(audio_Source*));
         sources_count--;
         break;
      }
   }

   slock_unlock(sources_lock);
}

void mixer_render(int16_t *buffer)
{
   uint8_t* rawsamples8     = NULL;
   size_t   rawsamples8_bps = 0;

   // Clear buffer
   memset(buffer, 0, AUDIO_FRAMES * 2 * sizeof(int16_t));

   /* Audio subsystem not ready */
   if (!sources_count || !sources)
      return;

   slock_lock(sources_lock);

   // Loop over audio sources
   for (unsigned i = 0; i < sources_count; i++)
   {
      if (sources[i]->state == AUDIO_STOPPED)
         continue;

      if (sources[i]->bps > rawsamples8_bps)
      {
         rawsamples8_bps = sources[i]->bps;
         rawsamples8     = realloc(rawsamples8, rawsamples8_bps * AUDIO_FRAMES);
         memset(rawsamples8, 0, rawsamples8_bps * AUDIO_FRAMES);
      }

      bool end = ! fread(rawsamples8,
            sizeof(uint8_t),
            AUDIO_FRAMES * sources[i]->bps,
            sources[i]->sndta.fp);

      int16_t* rawsamples16 = (int16_t*)rawsamples8;
      
      for (unsigned j = 0; j < AUDIO_FRAMES; j++)
      {
         int16_t left = 0;
         int16_t right = 0;
         if (sources[i]->sndta.head.NumChannels == 1 && sources[i]->sndta.head.BitsPerSample ==  8) { left = right = rawsamples8[j]*64; }
         if (sources[i]->sndta.head.NumChannels == 2 && sources[i]->sndta.head.BitsPerSample ==  8) { left = rawsamples8[j*2+0]*64; right=rawsamples8[j*2+1]*64; }
         if (sources[i]->sndta.head.NumChannels == 1 && sources[i]->sndta.head.BitsPerSample == 16) { left = right = rawsamples16[j]; }
         if (sources[i]->sndta.head.NumChannels == 2 && sources[i]->sndta.head.BitsPerSample == 16) { left = rawsamples16[j*2+0]; right=rawsamples16[j*2+1]; }
         buffer[j*2+0] += left  * sources[i]->volume * volume;
         buffer[j*2+1] += right * sources[i]->volume * volume;
      }

      if (end)
      {
         if (!sources[i]->loop)
            sources[i]->state = AUDIO_STOPPED;
         fseek(sources[i]->sndta.fp, WAV_HEADER_SIZE, SEEK_SET);
      }

   }

   if (rawsamples8)
      free(rawsamples8);

   slock_unlock(sources_lock);
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
   sources_lock = slock_new();
}

void lutro_audio_deinit()
{
   if (sources_lock)
      slock_lock(sources_lock);

   if (sources)
      free(sources);

   sources           = NULL;
   sources_count     = 0;
   sources_allocated = 0;

   if (sources_lock)
   {
      slock_t *tmp = sources_lock;
      sources_lock = NULL;

      slock_unlock(tmp);
      slock_free(tmp);
   }
}

int audio_newSource(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 1 && n != 2)
      return luaL_error(L, "lutro.audio.newSource requires 1 or 2 arguments, %d given.", n);

   audio_Source* self = (audio_Source*)lua_newuserdata(L, sizeof(audio_Source));

   void *p = lua_touserdata(L, 1);
   if (p == NULL)
   {
      const char* path = luaL_checkstring(L, 1);

      char fullpath[PATH_MAX_LENGTH];
      strlcpy(fullpath, settings.gamedir, sizeof(fullpath));
      strlcat(fullpath, path, sizeof(fullpath));

      FILE *fp = fopen(fullpath, "rb");
      if (!fp)
         return -1;

      fread(&self->sndta.head, sizeof(uint8_t), WAV_HEADER_SIZE, fp);
      self->sndta.fp = fp;
   }
   else
   {
      snd_SoundData* sndta = (snd_SoundData*)luaL_checkudata(L, 1, "SoundData");
      self->sndta = *sndta;
   }

   self->bps = self->sndta.head.NumChannels * self->sndta.head.BitsPerSample / 8;
   self->loop = false;
   self->volume = 1.0;
   self->state = AUDIO_STOPPED;
   fseek(self->sndta.fp, 0, SEEK_END);

   add_source(self);

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
   remove_source(self);

   self->state = AUDIO_STOPPED;

   /* TODO: do this at SoundData's gc method */
   if (self->sndta.fp)
      fclose(self->sndta.fp);

   self->sndta.fp   = NULL;

   return 0;
}

int audio_play(lua_State *L)
{
   audio_Source* self = (audio_Source*)luaL_checkudata(L, 1, "Source");
   bool success = fseek(self->sndta.fp, WAV_HEADER_SIZE, SEEK_SET) == 0;
   if (success)
      self->state = AUDIO_PLAYING;
   lua_pushboolean(L, success);
   return 1;
}

int audio_stop(lua_State *L)
{
   audio_Source* self = (audio_Source*)luaL_checkudata(L, 1, "Source");
   bool success = fseek(self->sndta.fp, WAV_HEADER_SIZE, SEEK_SET) == 0;
   if (success)
      self->state = AUDIO_STOPPED;
   lua_pushboolean(L, success);
   return 1;
}
