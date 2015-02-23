#include "audio.h"
#include "lutro.h"
#include "compat/strl.h"

#include <stdlib.h>
#include <string.h>

static unsigned num_sources = 0;
static audio_Source** sources = NULL;

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
         buffer[j*2+0] += rawsamples16[j];
         buffer[j*2+1] += rawsamples16[j];
      }

      if (end && sources[i]->loop)
         fseek(sources[i]->fp, WAV_HEADER_SIZE, SEEK_SET);

      free(rawsamples8);
   }
}

int lutro_audio_preload(lua_State *L)
{
   static luaL_Reg gfx_funcs[] =  {
      { "newSource",    audio_newSource },
      { "play",         audio_play },
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

   if (n != 1)
      return luaL_error(L, "lutro.audio.newSource requires 1 arguments, %d given.", n);

   const char* path = luaL_checkstring(L, 1);

   char fullpath[PATH_MAX_LENGTH];
   strlcpy(fullpath, settings.gamedir, sizeof(fullpath));
   strlcat(fullpath, path, sizeof(fullpath));

   audio_Source* self = (audio_Source*)lua_newuserdata(L, sizeof(audio_Source));

   FILE *fp = fopen(fullpath, "rb");
   if (!fp)
      return -1;

   wavhead_t head;
   fread(&head, sizeof(uint8_t), WAV_HEADER_SIZE, fp);
   self->bps = head.NumChannels * head.BitsPerSample / 8;
   self->fp = fp;
   self->loop = false;
   fseek(self->fp, 0, SEEK_END);

   num_sources++;
   sources = (audio_Source**)realloc(sources, num_sources * sizeof(audio_Source));
   sources[num_sources-1] = self;

   if (luaL_newmetatable(L, "Source") != 0)
   {
      static luaL_Reg audio_funcs[] = {
         { "__gc",  source_gc },
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

int source_gc(lua_State *L)
{
   audio_Source* self = (audio_Source*)luaL_checkudata(L, 1, "Source");
   (void)self;
   return 0;
}

int audio_play(lua_State *L)
{
   audio_Source* self = (audio_Source*)luaL_checkudata(L, 1, "Source");
   fseek(self->fp, WAV_HEADER_SIZE, SEEK_SET);
   return 0;
}
