#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "runtime.h"
#include "sound.h"

#define AUDIO_FRAMES (44100 / 60)

typedef enum
{
   AUDIO_STOPPED = 0,
   AUDIO_PAUSED,
   AUDIO_PLAYING
} audio_source_state;

typedef struct
{
   snd_SoundData sndta;
   unsigned bps; // bytes per sample
   bool loop;
   float volume;
   float pitch;
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

int source_setLooping(lua_State *L);
int source_isLooping(lua_State *L);
int source_isStopped(lua_State *L);
int source_isPaused(lua_State *L);
int source_isPlaying(lua_State *L);
int source_getVolume(lua_State *L);
int source_setVolume(lua_State *L);
int source_getPitch(lua_State *L);
int source_setPitch(lua_State *L);

int source_gc(lua_State *L);

#endif // AUDIO_H
