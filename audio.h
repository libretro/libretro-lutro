#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "runtime.h"
#include "sound.h"
#include "decoder.h"

#define AUDIO_FRAMES (44100 / 60)

// The following types are acceptable for pre-saturated mixing, as they meet the requirement for
// having a larger range than the saturated mixer result type of int16_t. double precision should
// be preferred on x86/amd64, and single precision on ARM. float16 could also work as an input
// but care must be taken to saturate at INT16_MAX-1 and INT16_MIN+1 due to float16 not having a
// 1:1 representation of whole numbers in the in16 range.
//
// TODO: set up appropriate compiler defs for mixer presaturate type.

typedef float   mixer_presaturate_t;
#define cvt_presaturate_to_int16(in)   ((int16_t)roundf(in))

//typedef double  mixer_presaturate_t;
//#define cvt_presaturate_to_int16(in)   (round(in))

//typedef int32_t mixer_presaturate_t;
//#define cvt_presaturate_to_int16(in)   ((int16_t)in)


typedef enum
{
   AUDIO_STOPPED = 0,
   AUDIO_PAUSED,
   AUDIO_PLAYING
} audio_source_state;

typedef struct
{
   //currently for WAV
   snd_SoundData sndta;
   unsigned bps; // bytes per sample
   
   //currently for Ogg Vorbis
   OggData *oggData;
   
   bool loop;
   float volume;
   float pitch;
   unsigned pos;
   audio_source_state state;
} audio_Source;

void lutro_audio_init();
void lutro_audio_deinit();
int lutro_audio_preload(lua_State *L);
void mixer_render(int16_t *buffer);

int audio_newSource(lua_State *L);
int audio_setVolume(lua_State *L);
int audio_getVolume(lua_State *L);
int audio_play(lua_State *L);
int audio_stop(lua_State *L);

int audio_sources_nullify_refs(const snd_SoundData* sndta);

int source_setLooping(lua_State *L);
int source_isLooping(lua_State *L);
int source_isStopped(lua_State *L);
int source_isPaused(lua_State *L);
int source_isPlaying(lua_State *L);
int source_getVolume(lua_State *L);
int source_setVolume(lua_State *L);
int source_tell(lua_State *L);
int source_seek(lua_State *L);
int source_getPitch(lua_State *L);
int source_setPitch(lua_State *L);

int source_gc(lua_State *L);


#endif // AUDIO_H
