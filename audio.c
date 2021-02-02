#include "audio.h"
#include "lutro.h"
#include "compat/strl.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <file/file_path.h>
#include <audio/conversion/float_to_s16.h>
#include <math.h>

/* TODO/FIXME - no sound on big-endian */

static unsigned num_sources = 0;
static audio_Source** sources = NULL;
static float volume = 1.0;

#define CHANNELS 2
float floatBuffer[AUDIO_FRAMES * CHANNELS];
int16_t convBuffer[AUDIO_FRAMES * CHANNELS];

// The following types are acceptable for pre-saturated mixing, as they meet the requirement for
// having a larger range than the saturated mixer result type of int16_t. double precision should
// be preferred on x86/amd64, and single precision on ARM. float16 could also work as an input
// but care must be taken to saturate at INT16_MAX-1 and INT16_MIN+1 due to float16 not having a
// 1:1 representation of whole numbers in the in16 range.
//
// TODO: set up appropriate compiler defs for mixer presaturate type.

typedef float   mixer_presaturate_t;
#define cvt_presaturate_to_int16(in)   (roundf(in))

//typedef double  mixer_presaturate_t;
//#define cvt_presaturate_to_int16(in)   (round(in))

//typedef int32_t mixer_presaturate_t;
//#define cvt_presaturate_to_int16(in)   ((int16_t)in)

static int16_t saturate(mixer_presaturate_t in) {
   if (in >=  INT16_MAX) { return INT16_MAX; }
   if (in <=  INT16_MIN) { return INT16_MIN; }
   return cvt_presaturate_to_int16(in);
}

void mixer_render(int16_t *buffer)
{
   // Clear buffers
   memset(buffer, 0, AUDIO_FRAMES * CHANNELS * sizeof(int16_t));
   memset(floatBuffer, 0, AUDIO_FRAMES * CHANNELS * sizeof(float));

   bool floatBufferUsed = false;

   // Loop over audio sources
   for (unsigned i = 0; i < num_sources; i++)
   {
      if (sources[i]->state == AUDIO_STOPPED)
         continue;

      //currently Ogg Vorbis
      if (sources[i]->oggData)
      {
         bool finished = decoder_decodeOgg(sources[i]->oggData, floatBuffer, sources[i]->volume, sources[i]->loop);
         if (finished)
            sources[i]->state = AUDIO_STOPPED;
         floatBufferUsed = true;
         continue;
      }
      
      uint8_t* rawsamples8 = calloc(
         AUDIO_FRAMES * sources[i]->bps, sizeof(uint8_t));

      fseek(sources[i]->sndta.fp, WAV_HEADER_SIZE + sources[i]->pos, SEEK_SET);

      bool end = ! fread(rawsamples8,
            sizeof(uint8_t),
            AUDIO_FRAMES * sources[i]->bps,
            sources[i]->sndta.fp);

      int16_t* rawsamples16 = (int16_t*)rawsamples8;

      for (unsigned j = 0; j < AUDIO_FRAMES; j++)
      {
         mixer_presaturate_t left = 0;
         mixer_presaturate_t right = 0;
         if (sources[i]->sndta.head.NumChannels == 1 && sources[i]->sndta.head.BitsPerSample ==  8) { left = right = rawsamples8[j]*64; }
         if (sources[i]->sndta.head.NumChannels == 2 && sources[i]->sndta.head.BitsPerSample ==  8) { left = rawsamples8[j*2+0]*64; right=rawsamples8[j*2+1]*64; }
         if (sources[i]->sndta.head.NumChannels == 1 && sources[i]->sndta.head.BitsPerSample == 16) { left = right = rawsamples16[j]; }
         if (sources[i]->sndta.head.NumChannels == 2 && sources[i]->sndta.head.BitsPerSample == 16) { left = rawsamples16[j*2+0]; right=rawsamples16[j*2+1]; }
         buffer[j*2+0] = saturate(buffer[j*2+0] + (left  * sources[i]->volume * volume));
         buffer[j*2+1] = saturate(buffer[j*2+1] + (right * sources[i]->volume * volume));
         sources[i]->pos += sources[i]->bps;
      }

      if (end)
      {
         if (!sources[i]->loop)
            sources[i]->state = AUDIO_STOPPED;
         fseek(sources[i]->sndta.fp, WAV_HEADER_SIZE, SEEK_SET);
         sources[i]->pos = 0;
      }

      free(rawsamples8);
   }

   //add in accumulated float buffer
   if (floatBufferUsed)
   {
      convert_float_to_s16(convBuffer, floatBuffer, AUDIO_FRAMES * CHANNELS); //convert to int
      for (unsigned j = 0; j < AUDIO_FRAMES * CHANNELS; j++)
         buffer[j] += convBuffer[j] * volume;
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
         if (sources[i]->oggData)
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

int audio_newSource(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 1 && n != 2)
      return luaL_error(L, "lutro.audio.newSource requires 1 or 2 arguments, %d given.", n);

   audio_Source* self = (audio_Source*)lua_newuserdata(L, sizeof(audio_Source));
   self->oggData = NULL;
   self->sndta.fp = NULL;        

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
         ext[i] = tolower(ext[i]);
      
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
   num_sources++;
   sources = (audio_Source**)realloc(sources, num_sources * sizeof(audio_Source));
   sources[num_sources-1] = self;

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
   lua_pushnumber(L, self->pos);
   return 1;
}

int source_seek(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 3)
      return luaL_error(L, "Source:seek requires 3 arguments, %d given.", n);

   audio_Source* self = (audio_Source*)luaL_checkudata(L, 1, "Source");
   self->pos = (unsigned)luaL_checknumber(L, 2);

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
   (void)self;
   return 0;
}

int audio_play(lua_State *L)
{
   audio_Source* self = (audio_Source*)luaL_checkudata(L, 1, "Source");

   bool success = false;

   //WAV file
   if (self->sndta.fp)
   {
      success = fseek(self->sndta.fp, WAV_HEADER_SIZE, SEEK_SET) == 0;
      if (success)
        self->state = AUDIO_PLAYING;
   }
   //OGG file
   else if (self->oggData)
   {
      success = decoder_seekStart(self->oggData);
      if (success)
         self->state = AUDIO_PLAYING;
   }
   
   lua_pushboolean(L, success);
   return 1;
}

int audio_stop(lua_State *L)
{
   audio_Source* self = (audio_Source*)luaL_checkudata(L, 1, "Source");
   bool success = false;

   //WAV file
   if (self->sndta.fp)
   {
      success = fseek(self->sndta.fp, WAV_HEADER_SIZE, SEEK_SET) == 0;
      if (success)
        self->state = AUDIO_STOPPED;
   }
   //OGG file
   else if (self->oggData)
   {
      success = decoder_seekStart(self->oggData);
      if (success)
         self->state = AUDIO_STOPPED;
   }
   
   lua_pushboolean(L, success);
   return 1;
}
