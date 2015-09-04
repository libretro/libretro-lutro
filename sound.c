#include "sound.h"
#include "lutro.h"
#include "compat/strl.h"

#include <stdlib.h>
#include <string.h>

int lutro_sound_preload(lua_State *L)
{
   static luaL_Reg snd_funcs[] =  {
      { "newSoundData", snd_newSoundData },
      {NULL, NULL}
   };

   lutro_ensure_global_table(L, "lutro");

   luaL_newlib(L, snd_funcs);

   lua_setfield(L, -2, "sound");

   return 1;
}

void lutro_sound_init()
{
}

int snd_newSoundData(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 1)
      return luaL_error(L, "lutro.sound.newSoundData requires 1 argument, %d given.", n);

   const char* path = luaL_checkstring(L, 1);

   char fullpath[PATH_MAX_LENGTH];
   strlcpy(fullpath, settings.gamedir, sizeof(fullpath));
   strlcat(fullpath, path, sizeof(fullpath));

   snd_SoundData* self = (snd_SoundData*)lua_newuserdata(L, sizeof(snd_SoundData));

   FILE *fp = fopen(fullpath, "rb");
   if (!fp)
      return -1;

   fread(&self->head, sizeof(uint8_t), WAV_HEADER_SIZE, fp);
   self->fp = fp;

   if (luaL_newmetatable(L, "SoundData") != 0)
   {
      static luaL_Reg sndta_funcs[] = {
         { "type", sndta_type },
         { "__gc", sndta_gc },
         {NULL, NULL}
      };

      lua_pushvalue(L, -1);

      lua_setfield(L, -2, "__index");

      lua_pushcfunction(L, sndta_gc);
      lua_setfield( L, -2, "__gc" );

      luaL_setfuncs(L, sndta_funcs, 0);
   }

   lua_setmetatable(L, -2);

   return 1;
}

int sndta_type(lua_State *L)
{
   wavhead_t* self = (wavhead_t*)luaL_checkudata(L, 1, "SoundData");
   (void) self;
   lua_pushstring(L, "SoundData");
   return 1;
}

int sndta_gc(lua_State *L)
{
   wavhead_t* self = (wavhead_t*)luaL_checkudata(L, 1, "SoundData");
   (void)self;
   return 0;
}
