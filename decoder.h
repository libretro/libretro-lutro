#ifndef DECODER_H
#define DECODER_H

#include <vorbis/vorbisfile.h>
#include "audio_mixer.h"

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
   OggVorbis_File vf;
   vorbis_info *info;
} dec_OggData;

typedef struct
{
   void* fp;
   int   pos;
   wavhead_t head;
} dec_WavData;

bool decWav_init(dec_WavData *data, const char *filename);
void decWav_destroy(dec_WavData *data);
bool decWav_seek(dec_WavData *data, intmax_t pos);
intmax_t decWav_sampleTell(dec_WavData *data);
bool decWav_decode(dec_WavData *data, presaturate_buffer_desc *buffer, float volume, bool loop);

bool decoder_initOgg(dec_OggData *data, const char *filename);
void decoder_destroyOgg(dec_OggData *data);
bool decoder_seek(dec_OggData *data, intmax_t pos);
intmax_t decoder_sampleTell(dec_OggData *data);
intmax_t decoder_sampleLength(dec_OggData *data);
bool decoder_decodeOgg(dec_OggData *data, presaturate_buffer_desc *buffer, float volume, bool loop);

#endif // DECODER_H
