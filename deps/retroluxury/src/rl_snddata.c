#include <rl_snddata.h>
#include <rl_resample.h>

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <rl_endian.inl>

typedef struct
{
  /* header */
  char     chunkid[ 4 ];
  uint32_t chunksize;
  char     format[ 4 ];
  /* fmt */
  char     subchunk1id[ 4 ];
  uint32_t subchunk1size;
  uint16_t audioformat;
  uint16_t numchannels;
  uint32_t samplerate;
  uint32_t byterate;
  uint16_t blockalign;
  uint16_t bitspersample;
  /* data */
  char     subchunk2id[ 4 ];
  uint32_t subchunk2size;
  uint8_t  data[ 0 ];
  
}
wave_t;

int rl_snddata_create( rl_snddata_t* snddata, const void* data, size_t size )
{
  const wave_t* wave = (const wave_t*)data;
  
  if ( wave->chunkid[ 0 ] != 'R' || wave->chunkid[ 1 ] != 'I' || wave->chunkid[ 2 ] != 'F' || wave->chunkid[ 3 ] != 'F' )
  {
    return -1;
  }
  
  if ( wave->format[ 0 ] != 'W' || wave->format[ 1 ] != 'A' || wave->format[ 2 ] != 'V' || wave->format[ 3 ] != 'E' )
  {
    return -1;
  }
  
  if ( wave->subchunk1id[ 0 ] != 'f' || wave->subchunk1id[ 1 ] != 'm' || wave->subchunk1id[ 2 ] != 't' || wave->subchunk1id[ 3 ] != ' ' )
  {
    return -1;
  }
  
  if ( wave->subchunk2id[ 0 ] != 'd' || wave->subchunk2id[ 1 ] != 'a' || wave->subchunk2id[ 2 ] != 't' || wave->subchunk2id[ 3 ] != 'a' )
  {
    return -1;
  }
  
  snddata->bps = le16( wave->bitspersample );
  snddata->channels = le16( wave->numchannels );
  snddata->freq = le32( wave->samplerate );
  
  if ( le16( wave->audioformat ) != 1 )
  {
    return -1;
  }
  
  if ( snddata->channels != 1 && snddata->channels != 2 )
  {
    return -1;
  }
  
  if ( snddata->bps != 8 && snddata->bps != 16 )
  {
    return -1;
  }
  
  snddata->size = le32( wave->subchunk2size );
  snddata->samples = malloc( snddata->size );
  
  if ( snddata->samples )
  {
    memcpy( (void*)snddata->samples, (void*)wave->data, snddata->size );
    return 0;
  }
  
  return -1;
}

const int16_t* rl_snddata_encode( size_t* out_samples, const rl_snddata_t* snddata )
{
  int16_t* out_buffer;
  size_t in_samples;
  
  if ( snddata->bps == 8 )
  {
    if ( snddata->channels == 1 )
    {
      /* 8 bps, mono. */
      in_samples = snddata->size;
      *out_samples = in_samples * 2;
      out_buffer = (int16_t*)malloc( *out_samples * sizeof( int16_t ) );
      
      if ( !out_buffer )
      {
        return NULL;
      }
      
      int16_t* samples = out_buffer;
      const uint8_t* begin = (const uint8_t*)snddata->samples;
      const uint8_t* end = begin + in_samples;
      
      while ( begin < end )
      {
        int sample = *begin++;
        sample = sample * 65792 / 256 - 32768;
        *samples++ = sample;
        *samples++ = sample;
      }
    }
    else
    {
      /* 8 bps, stereo. */
      in_samples = snddata->size;
      *out_samples = in_samples;
      out_buffer = (int16_t*)malloc( *out_samples * sizeof( int16_t ) );
      
      if ( !out_buffer )
      {
        return NULL;
      }
      
      int16_t* samples = out_buffer;
      const uint8_t* begin = (const uint8_t*)snddata->samples;
      const uint8_t* end = begin + in_samples;
      
      while ( begin < end )
      {
        int sample = *begin++;
        sample = sample * 65792 / 256 - 32768;
        *samples++ = sample;
      }
    }
  }
  else
  {
    if ( snddata->channels == 1 )
    {
      /* 16 bps, mono. */
      in_samples = snddata->size / 2;
      *out_samples = in_samples * 2;
      out_buffer = (int16_t*)malloc( *out_samples * sizeof( int16_t ) );
      
      if ( !out_buffer )
      {
        return NULL;
      }
      
      int16_t* samples = out_buffer;
      const uint16_t* begin = (const uint16_t*)snddata->samples;
      const uint16_t* end = begin + in_samples;
      
      while ( begin < end )
      {
        int16_t sample = le16( *begin++ );
        *samples++ = sample;
        *samples++ = sample;
      }
    }
    else
    {
      /* 16 bps, stereo. */
      in_samples = snddata->size / 2;
      *out_samples = in_samples;
      out_buffer = (int16_t*)malloc( *out_samples * sizeof( int16_t ) );
      
      if ( !out_buffer )
      {
        return NULL;
      }
      
      int16_t* samples = out_buffer;
      const uint16_t* begin = (const uint16_t*)snddata->samples;
      const uint16_t* end = begin + in_samples;
      
      while ( begin < end )
      {
        *samples++ = le16( *begin++ );
      }
    }
  }
  
  if ( snddata->freq != RL_SAMPLE_RATE )
  {
    rl_resampler_t* resampler = rl_resampler_create( snddata->freq );
    
    if ( !resampler )
    {
      free( out_buffer );
      return NULL;
    }
    
    size_t new_samples = rl_resampler_out_samples( resampler, *out_samples );
    int16_t* new_buffer = (int16_t*)malloc( new_samples * sizeof( int16_t ) );
    
    if ( !new_buffer )
    {
      rl_resampler_destroy( resampler );
      free( out_buffer );
      return NULL;
    }
    
    if ( rl_resampler_resample( resampler, out_buffer, *out_samples, new_buffer, new_samples ) != new_samples )
    {
      free( new_buffer );
      rl_resampler_destroy( resampler );
      free( out_buffer );
      return NULL;
    }
    
    free( out_buffer );
    out_buffer = new_buffer;
    *out_samples = new_samples;
  }
  
  return out_buffer;
}
