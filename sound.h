#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "runtime.h"

#include "audio_mixer.h"

// pre-decoded soundData.
//
// it is always matching our internal mixer_presaturate_t type, so there's no
// need to track bytes per sample.
typedef struct
{
   int numChannels;
   intmax_t numSamples;
   mixer_presaturate_t* data;
} snd_SoundData;

void lutro_sound_init(void);
int lutro_sound_preload(lua_State *L);

int snd_newSoundData(lua_State *L);

int sndta_type(lua_State *L);
int sndta_gc(lua_State *L);

#endif // SOUND_H
