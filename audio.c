#include "audio.h"
#include "lutro.h"
#include "compat/strl.h"

#include <stdlib.h>
#include <string.h>

/* TODO/FIXME - no sound on big-endian */

static unsigned num_sources = 0;
static audio_Source** sources = NULL;
static float volume = 1.0;

void mixer_render(int16_t *buffer)
{
   // Clear buffer
   for (unsigned j = 0; j < AUDIO_FRAMES; j++)
      buffer[j*2+0] = buffer[j*2+1] = 0;

   // Loop over audio sources
   for (unsigned i = 0; i < num_sources; i++)
   {
      uint8_t* rawsamples8 = calloc(
         AUDIO_FRAMES * sources[i]->bps, sizeof(uint8_t));

      bool end = ! fread(rawsamples8,
            sizeof(uint8_t),
            AUDIO_FRAMES * sources[i]->bps,
            sources[i]->fp);

      int16_t* rawsamples16 = (int16_t*)rawsamples8;
      
      for (unsigned j = 0; j < AUDIO_FRAMES; j++)
      {
         int16_t left = 0;
         int16_t right = 0;
         if (sources[i]->head.NumChannels == 1 && sources[i]->head.BitsPerSample ==  8) { left = right = rawsamples8[j]*64; }
         if (sources[i]->head.NumChannels == 2 && sources[i]->head.BitsPerSample ==  8) { left = rawsamples8[j*2+0]*64; right=rawsamples8[j*2+1]*64; }
         if (sources[i]->head.NumChannels == 1 && sources[i]->head.BitsPerSample == 16) { left = right = rawsamples16[j]; }
         if (sources[i]->head.NumChannels == 2 && sources[i]->head.BitsPerSample == 16) { left = rawsamples16[j*2+0]; right=rawsamples16[j*2+1]; }
         buffer[j*2+0] += left  * sources[i]->volume * volume;
         buffer[j*2+1] += right * sources[i]->volume * volume;
      }

      if (end && sources[i]->loop)
         fseek(sources[i]->fp, WAV_HEADER_SIZE, SEEK_SET);

      free(rawsamples8);
   }
}

int lutro_audio_preload(lua_State *L)
{
   static luaL_Reg gfx_funcs[] =  {
      { "getVolume",    audio_getVolume },
      { "newSource",    audio_newSource },
      { "play",         audio_play },
      { "setVolume",    audio_setVolume },
      {NULL, NULL}
   };

   lutro_ensure_global_table(L, "lutro");

   luaL_newlib(L, gfx_funcs);

   lua_setfield(L, -2, "audio");

   return 1;
}

void lutro_audio_init()
{
}

int audio_newSource(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 1 && n != 2)
      return luaL_error(L, "lutro.audio.newSource requires 1 or 2 arguments, %d given.", n);

   const char* path = luaL_checkstring(L, 1);

   char fullpath[PATH_MAX_LENGTH];
   strlcpy(fullpath, settings.gamedir, sizeof(fullpath));
   strlcat(fullpath, path, sizeof(fullpath));

   audio_Source* self = (audio_Source*)lua_newuserdata(L, sizeof(audio_Source));

   FILE *fp = fopen(fullpath, "rb");
   if (!fp)
      return -1;

   fread(&self->head, sizeof(uint8_t), WAV_HEADER_SIZE, fp);
   self->bps = self->head.NumChannels * self->head.BitsPerSample / 8;
   self->fp = fp;
   self->loop = false;
   self->volume = 1.0;
   self->state = AUDIO_STOPPED;
   fseek(self->fp, 0, SEEK_END);

   num_sources++;
   sources = (audio_Source**)realloc(sources, num_sources * sizeof(audio_Source));
   sources[num_sources-1] = self;

   if (luaL_newmetatable(L, "Source") != 0)
   {
      static luaL_Reg audio_funcs[] = {
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
      return luaL_error(L, "Source.setVolume requires 2 arguments, %d given.", n);

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
      return luaL_error(L, "Source.setPitch requires 2 arguments, %d given.", n);

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
   (void)self;
   return 0;
}

int audio_play(lua_State *L)
{
   audio_Source* self = (audio_Source*)luaL_checkudata(L, 1, "Source");
   bool success = fseek(self->fp, WAV_HEADER_SIZE, SEEK_SET) == 0;
   if (success)
      self->state = AUDIO_PLAYING;
   lua_pushboolean(L, success);
   return 1;
}
