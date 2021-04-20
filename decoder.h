#ifndef DECODER_H
#define DECODER_H

#include <vorbis/vorbisfile.h>
#include "audio_mixer.h"

#define WAV_HEADER_CHUNK1_SIZE 36      // minimum size. the chunk can be larger. See Subchunk1Size.
#define WAV_HEADER_CHUNK2_SIZE 8

typedef struct
{
   char     ChunkID[4];
   uint32_t ChunkSize;
   char     Format[4];
   char     Subchunk1ID[4];
   uint32_t Subchunk1Size;
   uint16_t AudioFormat;
   uint16_t NumChannels;
   uint32_t SampleRate;
   uint32_t ByteRate;
   uint16_t BlockAlign;
   uint16_t BitsPerSample;
} wavhead_t;

typedef struct
{
   char Subchunk2ID[4];
   uint32_t Subchunk2Size;
} wav_subchunk2_t;

typedef struct
{
   OggVorbis_File vf;
   vorbis_info*   info;
} dec_OggData;

typedef struct
{
   void*           fp;
   intmax_t        pos;
   wavhead_t       headc1;             // RIFF header and Chunk 1
   wav_subchunk2_t headc2;             // data chunk header (other headers are not tracked)
   intmax_t        seekPosSubChunk2;   // data subchunk could be anywhere in this evil file format.
} dec_WavData;

bool decWav_init(dec_WavData *data, const char *filename);
void decWav_destroy(dec_WavData *data);
bool decWav_seek(dec_WavData *data, intmax_t pos);
intmax_t decWav_sampleTell(dec_WavData *data);
bool decWav_decode(dec_WavData *data, presaturate_buffer_desc *buffer, float volume, bool loop);

bool decOgg_init(dec_OggData *data, const char *filename);
void decOgg_destroy(dec_OggData *data);
bool decOgg_seek(dec_OggData *data, intmax_t pos);
intmax_t decOgg_sampleTell(dec_OggData *data);
intmax_t decOgg_sampleLength(dec_OggData *data);
bool decOgg_decode(dec_OggData *data, presaturate_buffer_desc *buffer, float volume, bool loop);

#endif // DECODER_H
