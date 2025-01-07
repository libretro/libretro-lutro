#include "sound.h"
#include "lutro.h"
#include "audio.h"
#include "compat/strl.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

int lutro_sound_preload(lua_State *L)
{
   static const luaL_Reg snd_funcs[] =  {
      { "newSoundData", snd_newSoundData },
      {NULL, NULL}
   };

   lutro_newlib(L, snd_funcs, "sound");

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

   AssetPathInfo asset;
   lutro_assetPath_init(&asset, path);

   snd_SoundData* self = (snd_SoundData*)lua_newuserdata(L, sizeof(snd_SoundData));

   presaturate_buffer_desc bufdesc;

   if (strstr(asset.ext, "ogg"))
   {
      dec_OggData oggData;
      decOgg_init(&oggData, asset.fullpath);
      self->numSamples  = decOgg_sampleLength(&oggData);
      self->numChannels = oggData.info->channels;
      self->data = lutro_calloc(1, sizeof(mixer_presaturate_t) * self->numSamples * self->numChannels);

      bufdesc.data      = self->data;
      bufdesc.channels  = self->numChannels;
      bufdesc.samplelen = self->numSamples;

      decOgg_decode(&oggData, &bufdesc, 1.0f, false);
      decOgg_destroy(&oggData);
   }

   if (strstr(asset.ext, "wav"))
   {
      dec_WavData wavData;
      decWav_init(&wavData, asset.fullpath);
      self->numSamples  = wavData.headc2.Subchunk2Size / ((wavData.headc1.BitsPerSample/8) * wavData.headc1.NumChannels);
      self->numChannels = wavData.headc1.NumChannels;
      self->data = lutro_calloc(1, sizeof(mixer_presaturate_t) * self->numSamples * self->numChannels);

      bufdesc.data      = self->data;
      bufdesc.channels  = self->numChannels;
      bufdesc.samplelen = self->numSamples;

      decWav_decode(&wavData, &bufdesc, 1.0f, false);
      decWav_destroy(&wavData);
   }

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
   snd_SoundData* self = (snd_SoundData*)luaL_checkudata(L, 1, "SoundData");
   (void) self;
   lua_pushstring(L, "SoundData");
   return 1;
}

int sndta_gc(lua_State *L)
{
   snd_SoundData* self = (snd_SoundData*)luaL_checkudata(L, 1, "SoundData");
   lutro_free(self->data);
   self->data = NULL;

   // audio makes deep copies of this object when it preps it as a mixer source, so no mixer cleanup needed here.
   return 0;
}
