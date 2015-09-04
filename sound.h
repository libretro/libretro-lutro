#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "runtime.h"

#define WAV_HEADER_SIZE 44

typedef struct
{
   char ChunkID[4];
   uint32_t ChunkSize;
   char Format[4];
   char Subchunk1ID[4];
   uint32_t Subchunk1Size;
   uint16_t AudioFormat;
   uint16_t NumChannels;
   uint32_t SampleRate;
   uint32_t ByteRate;
   uint16_t BlockAlign;
   uint16_t BitsPerSample;
   char Subchunk2ID[4];
   uint32_t Subchunk2Size;
} wavhead_t;

typedef struct
{
   void* fp;
   wavhead_t head;
} snd_SoundData;

void lutro_sound_init();
int lutro_sound_preload(lua_State *L);
void mixer_render(int16_t *buffer);

int snd_newSoundData(lua_State *L);

int sndta_type(lua_State *L);
int sndta_gc(lua_State *L);

#endif // SOUND_H
