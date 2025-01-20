#include <libretro.h>
#include <audio/audio_mix.h>
#include <audio/conversion/float_to_s16.h>
#include <errno.h>
#include <string.h>

#include "lutro_assert.h"
#include "decoder.h"
#include "audio.h"

#if !defined(__always_inline)
#  ifdef _MSC_VER
#    define __always_inline  // microsoft has no equivalent to always_inline in C
#  else
#    define __always_inline  __attribute((always_inline))
#  endif
#endif

// the exposed Decoder API in Love2D is pretty much garbage and I don't think we should
// make any attempt to replicate it here (there are plenty of GH issues complaining that it
// can't realoly do anything, and it has some nonsense things like 'seek' that aren't clear
// if they impact sources or not). --jstine
//
// This file follows Decoder slightly for historical reasons but should really be thought
// of as a streaming Source API.

//
void decOgg_destroy(dec_OggData *data)
{
   ov_clear(&data->vf);
}

//
bool decOgg_init(dec_OggData *data, const char *filename)
{
   memset(data, 0, sizeof(*data));

   if (ov_fopen(filename, &data->vf) < 0)
   {
      // only print for file not found errors.
      // caller's intent might be to silently try opening "optional" files until one succeeeds.
      if (errno == ENOENT)
      {
         fprintf(stderr, "vorbis file not found: %s\n", filename);
         return false;
      }

      lutro_errorf("vorbis: Failed to open file: %s: %s", filename, strerror(errno));
      return false;
   }

   // printf("vorbis info:\n");
   // printf("\tnum streams: %d\n",  ov_streams(&data->vf));

   data->info = (vorbis_info*)ov_info(&data->vf, 0);
   if (!data->info)
   {
      lutro_errorf("vorbis: couldn't get info for file: %s", filename);
      return false;
   }

   // printf("\tnum channels: %d\n", data->info->channels);
   // printf("\tsample rate: %d\n", data->info->rate);

   if (data->info->channels != 1 && data->info->channels != 2)
   {
      lutro_errorf("vorbis: unsupported number of channels");
      return false;
   }

   if (data->info->rate != 44100)
   {
      lutro_errorf("vorbis: unsupported sample rate");
      return false;
   }

   // printf("vorbis init success\n");
   return true;
}

//
bool decOgg_seek(dec_OggData *data, intmax_t pos)
{
   // ogg doesn't do a cheap tell-check before invoking a very expensive seek operation internally,
   // so let's help it out here...
   if (ov_pcm_tell(&data->vf) != pos) {
      return ov_pcm_seek(&data->vf, pos) == 0;
   }
   return true;
}

//
intmax_t decOgg_sampleTell(dec_OggData *data)
{
   return ov_pcm_tell(&data->vf);
}

//
intmax_t decOgg_sampleLength(dec_OggData *data)
{
   return ov_pcm_total(&data->vf, -1);
}

// decoded data is mixed (added) into the presaturated mixer buffer.
// the buffer must be manually cleared to zero for non-mixing (raw) use cases.
bool decOgg_decode(dec_OggData *data, presaturate_buffer_desc *buffer, float volume, bool loop)
{
   bool finished = false;

   size_t rendered = 0;
   intmax_t bufsz = buffer->samplelen;
   mixer_presaturate_t* dst = buffer->data;

   while (bufsz)
   {
      float **pcm;
      int bitstream;
      // printf("pcmoffs: %d\n", data->vf.pcm_offset);
      intmax_t ret = ov_read_float(&data->vf, &pcm, bufsz, &bitstream);

      if (ret < 0)
      {
         lutro_errorf("Vorbis decoding failed with: %jd", ret);
         return true;
      }

      if (ret == 0) // EOF
      {
         if (loop)
         {
            if (ov_time_seek(&data->vf, 0.0) == 0)
               continue;
            else
               finished = true;
         }
         else
            finished = true;

         break;
      }

      if (data->info->channels == 2)
      {
         if (buffer->channels == 1)
         {
            // correct downmixing of stereo to mono is actually quite tricky, and requires advanced
            // waveform analysis to maintain volume and avoid cancelling out, but for now this will suffice. --jstine
            for (long i = 0; i < ret; i++)
            {
               dst[i] += pcm[0][i] * volume;
               dst[i] += pcm[1][i] * volume;
            }
         }

         if (buffer->channels == 2)
         {
            for (long i = 0; i < ret; i++)
            {
               dst[(i * 2) + 0] += pcm[0][i] * volume;
               dst[(i * 2) + 1] += pcm[1][i] * volume;
            }
         }
      }
      else
      {
         if (buffer->channels == 1)
         {
            for (long i = 0; i < ret; i++)
            {
               dst[i] += pcm[0][i] * volume;
            }
         }

         if (buffer->channels == 2)
         {
            for (long i = 0; i < ret; i++)
            {
               dst[(i * 2) + 0] += pcm[0][i] * volume;
               dst[(i * 2) + 1] += pcm[0][i] * volume;
            }
         }
      }

      dst += ret * data->info->channels;
      bufsz -= ret;
      rendered += ret;
   }

   return finished;
}

// ===================================== WAVFILE =====================================

void decWav_destroy(dec_WavData *data)
{
   if (data->fp)
   {
      fclose(data->fp);
      data->fp = NULL;
   }
}

//
bool decWav_init(dec_WavData *data, const char *filename)
{
   memset(data, 0, sizeof(*data));

   FILE *fp = fopen(filename, "rb");
   if (!fp)
   {
      int err = errno;

      // only print for file not found errors.
      // caller's intent might be to silently try opening "optional" files until one succeeeds.
      if (errno == ENOENT)
      {
         fprintf(stderr, "wavfile not found: %s\n", filename);
         return 0;
      }

      lutro_errorf("Failed to open wavfile '%s': %s", filename, strerror(err));
      return 0;
   }

   fread(&data->headc1, WAV_HEADER_CHUNK1_SIZE, 1, fp);

   if (feof(fp)
      || memcmp(data->headc1.ChunkID,      "RIFF", 4)
      || memcmp(data->headc1.Format,       "WAVE", 4)
      || memcmp(data->headc1.Subchunk1ID,  "fmt ", 4)
   ) {
      lutro_errorf("%s is not a valid wav file or is truncated.", filename);
      fclose(fp);
      return 0;
   }

   if (data->headc1.Subchunk1Size < 16)
   {
      lutro_errorf("%s has invalid subchunk size=%u. Expected size >= 16.", filename, data->headc1.Subchunk1Size);
      fclose(fp);
      return 0;
   }

   if (data->headc1.Subchunk1Size != 16)
   {
      int extra = data->headc1.Subchunk1Size - 16;
      fseek(fp, extra, SEEK_CUR);
   }

   while (1)
   {
      if (fread(&data->headc2, WAV_HEADER_CHUNK2_SIZE, 1, fp) == 0)
      {
         lutro_errorf("%s is not a supported wav file. No data subchunk was found.", filename);
         return 0;
      }

      if (memcmp(data->headc2.Subchunk2ID, "data", 4) == 0)
      {
         data->seekPosSubChunk2 = ftell(fp);
         data->fp = fp;
         return 1;
      }

      fseek(fp, data->headc2.Subchunk2Size, SEEK_CUR);
   }
   dbg_assumef(false, "unreachable");
   return 0;
}

int decWav_CalcOffsetDataStart(const dec_WavData* wavData)
{
   return wavData ? wavData->seekPosSubChunk2 : 0;
}

//
bool decWav_seek(dec_WavData *data, intmax_t samplepos)
{
   int bps = ((data->headc1.BitsPerSample + 7) / 8) * data->headc1.NumChannels;
   int numSamples = data->headc2.Subchunk2Size / bps;

   // fseek will let us seek past the end of file without returning an error.
   // So it is best to verify positions against the know sample size.
   // TODO: Verify Love2D Behavior? Love2D doesn't specify behavior in this case.
   //       Options are set the seekpos to 0, or set the seekpos to numSamples.

   if (samplepos > numSamples)
   {
      // set to numSamples
      //  * if the sample is set to loop it will restart immediately from loop pos
      //  * If not set to loop, it will stop immediately.
      samplepos = numSamples;
   }

   intmax_t bytepos = samplepos * bps;
   intmax_t seekpos = decWav_CalcOffsetDataStart(data) + bytepos;

   // spurious calls to fseek have overhead, so early out if the internal managed
   // seek pos matches
   if (data->pos == bytepos)
   {
      tool_assert(ftell(data->fp) == seekpos);
      return 1;
   }

   if (fseek(data->fp, seekpos, SEEK_SET))
   {
      // logging here could be unnecessarily spammy. If we add a log it should be gated by
      // some audio diagnostic output switch/mode.
      //fprintf(stderr, "WAV decoder seek failed: %s\n", strerror(errno));
      return 0;
   }
   data->pos = bytepos;
   return 1;
}

//
intmax_t decWav_sampleTell(dec_WavData *data)
{
   int bps = ((data->headc1.BitsPerSample + 7) / 8) * data->headc1.NumChannels;

   intmax_t ret = ftell(data->fp) - decWav_CalcOffsetDataStart(data);
   if (ret >= 0)
   {
      if ((ret % bps) != 0)
      {
         // print size, it helps identify the offender.
         fprintf(stderr, "Unaligned read position in wav decoder stream. size=%u bps=%u channels=%d pos=%jd\n",
            data->headc2.Subchunk2Size,
            data->headc1.BitsPerSample,
            data->headc1.NumChannels,
            ret
         );
      }
   }
   return ret / bps;
}

static __always_inline int inl_get_sample(const uint8_t* sample_raw, int sz, int chan)
{
   if(sz == 1)
      return ((int)sample_raw[chan] - 128) * 128;

   if(sz == 2)
      return (int)(((int16_t*)sample_raw)[chan]);

   return 0;
}

// this is a pseudo template with several cont literal parameters.
// it should always be agressively inlined.
static __always_inline bool _inl_decode_wav(dec_WavData *data, intmax_t bufsz, mixer_presaturate_t* dst, int bytesPerSamplePerChan, int chan_src, int chan_dst, float volume, bool loop)
{
   // a normalized sound sample is considered range -1.0 to 1.0
   // 16-bit wav outputs values range 32767 to -32768
   // 8-bit wav is scaled up to 16 bit and then normalized using 16-bit divisor.
   float mul_volume_and_normalize = volume / 32767;

   int bytesPerMultiSample = bytesPerSamplePerChan * chan_src;

   int byteLen    = data->headc2.Subchunk2Size;
   int numSamples = data->headc2.Subchunk2Size / bytesPerMultiSample;

   for (int j = 0; j < bufsz; j++, data->pos += bytesPerMultiSample)
   {
      uint8_t sample_raw[8];
      int readResult = 0;

   reloadSample:
      if (data->pos < byteLen)
         readResult = (int)fread(sample_raw, bytesPerMultiSample, 1, data->fp);

      if (!readResult)
      {
         dbg_assertf(data->pos == ftell(data->fp) - decWav_CalcOffsetDataStart(data), "numSamples=%jd byteLen=%jd dataPos=%jd and ftell=%jd",
            (intmax_t)numSamples, (intmax_t)byteLen, (intmax_t)data->pos, (intmax_t)ftell(data->fp)
         );

         if (!loop)
         {
            // love2D does not specify if seek/tell position should reset to zero or
            // point to the position past the last sample when a sample reaches its end.
            // Assuming ftell (position past end of stream) for now ...
            return 1;
         }

         data->pos = 0;
         fseek(data->fp, decWav_CalcOffsetDataStart(data), SEEK_SET);
         --j; goto reloadSample;    // attempt to re-read sample.
      }

      if (chan_src == 2)
      {
         if (chan_dst == 1)
         {
            dst[j] += inl_get_sample(sample_raw, bytesPerSamplePerChan, 0) * mul_volume_and_normalize;
            dst[j] += inl_get_sample(sample_raw, bytesPerSamplePerChan, 1) * mul_volume_and_normalize;
         }

         if (chan_dst == 2)
         {
            dst[(j*2)+0] += inl_get_sample(sample_raw, bytesPerSamplePerChan, 0) * mul_volume_and_normalize;
            dst[(j*2)+1] += inl_get_sample(sample_raw, bytesPerSamplePerChan, 1) * mul_volume_and_normalize;
         }
      }

      if (chan_src == 1)
      {
         if (chan_dst == 1)
         {
            dst[j] += inl_get_sample(sample_raw, bytesPerSamplePerChan, 0) * mul_volume_and_normalize;
         }

         if (chan_dst == 2)
         {
            dst[(j*2)+0] += inl_get_sample(sample_raw, bytesPerSamplePerChan, 0) * mul_volume_and_normalize;
            dst[(j*2)+1] += inl_get_sample(sample_raw, bytesPerSamplePerChan, 0) * mul_volume_and_normalize;
         }
      }
   }

   dbg_assertf(data->pos == ftell(data->fp) - decWav_CalcOffsetDataStart(data), "numSamples=%jd byteLen=%jd dataPos=%jd and ftell=%jd",
      (intmax_t)numSamples, (intmax_t)byteLen, (intmax_t)data->pos, (intmax_t)ftell(data->fp)
   );
   return 0;
}

// decoded data is mixed (added) into the presaturated mixer buffer.
// the buffer must be manually cleared to zero for non-mixing (raw) use cases.
bool decWav_decode(dec_WavData *data, presaturate_buffer_desc *buffer, float volume, bool loop)
{
   intmax_t bufsz = buffer->samplelen;
   mixer_presaturate_t* dst = buffer->data;

   // 8-bit samples are multiplied by 128 rather than the more mathematically appropriate 256 due to a
   // precedent in the authoring of 8-bit samples: due to their limited range of data, 8-bit samples
   // tend to be recorded at higher valumes in order to make full use of the dynamic range allowed, and
   // to avoid "hiss" that plagues 8-bit at low volumes. Therefore, as a rule of thumb, 8-bit samples
   // mixed at half volume will "match" better with 16-bit samples mixed at full volume. --jstine

   if (data->headc1.BitsPerSample == 8  && data->headc1.NumChannels == 2 && buffer->channels == 2) return _inl_decode_wav(data, bufsz, dst, 1, 2, 2, volume, loop);
   if (data->headc1.BitsPerSample == 8  && data->headc1.NumChannels == 2 && buffer->channels == 1) return _inl_decode_wav(data, bufsz, dst, 1, 2, 1, volume, loop);
   if (data->headc1.BitsPerSample == 8  && data->headc1.NumChannels == 1 && buffer->channels == 2) return _inl_decode_wav(data, bufsz, dst, 1, 1, 2, volume, loop);
   if (data->headc1.BitsPerSample == 8  && data->headc1.NumChannels == 1 && buffer->channels == 1) return _inl_decode_wav(data, bufsz, dst, 1, 1, 1, volume, loop);

   if (data->headc1.BitsPerSample == 16 && data->headc1.NumChannels == 2 && buffer->channels == 2) return _inl_decode_wav(data, bufsz, dst, 2, 2, 2, volume, loop);
   if (data->headc1.BitsPerSample == 16 && data->headc1.NumChannels == 2 && buffer->channels == 1) return _inl_decode_wav(data, bufsz, dst, 2, 2, 1, volume, loop);
   if (data->headc1.BitsPerSample == 16 && data->headc1.NumChannels == 1 && buffer->channels == 2) return _inl_decode_wav(data, bufsz, dst, 2, 1, 2, volume, loop);
   if (data->headc1.BitsPerSample == 16 && data->headc1.NumChannels == 1 && buffer->channels == 1) return _inl_decode_wav(data, bufsz, dst, 2, 1, 1, volume, loop);

   // TODO: some unsupported format (32-bit float?)
   // Return true to ensure the mixer stops and unrefs the sound data
   return true;
}
