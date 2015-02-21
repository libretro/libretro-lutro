#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "runtime.h"

#define AUDIO_FRAMES (44100 / 60)
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
   wavhead_t head;
   unsigned bps; // bytes per sample
   void* fp;
   bool loop;
} audio_Source;

void lutro_audio_init();
int lutro_audio_preload(lua_State *L);

int audio_newSource(lua_State *L);
int audio_play(lua_State *L);
int source_gc(lua_State *L);
void mixer_render();

#endif // AUDIO_H
