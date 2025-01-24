#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "runtime.h"
#include "sound.h"
#include "decoder.h"
#include "audio_mixer.h"

typedef enum
{
   AUDIO_STOPPED = 0,
   AUDIO_PAUSED,
   AUDIO_PLAYING
} audio_source_state;

typedef struct
{
   // only one of these should be non-null for a given source.

   dec_WavData*   wavData;       // streaming from wav
   dec_OggData*   oggData;       // streaming from ogg

   snd_SoundData* sndta;         // pre-decoded sound
   int lua_ref_sndta;            // (REGISTRY) ref to sndta is held as long as this object isn't disposed/__gc'd

   intmax_t sndpos;              // readpos in samples for pre-decoded sound only

   bool loop;
   float volume;
   float pitch;
   audio_source_state state;
} audio_Source;

void lutro_audio_init(lua_State* L);
void lutro_audio_deinit(void);
void lutro_audio_stop_all(lua_State* L);
int lutro_audio_preload(lua_State *L);
void lutro_mixer_render(int16_t* buffer);

void mixer_render(lua_State* L, int16_t *buffer);
void mixer_unref_stopped_sounds(lua_State* L);

int audio_newSource(lua_State *L);
int audio_setVolume(lua_State *L);
int audio_getVolume(lua_State *L);
int audio_play(lua_State *L);
int audio_stop(lua_State *L);
int audio_pause(lua_State *L);
int audio_getActiveSources(lua_State *L);
int audio_getActiveSourceCount(lua_State *L);

int source_pause(lua_State *L);
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
