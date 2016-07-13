#include <rl_resample.h>

#include <stdlib.h>
#include <string.h>

/*---------------------------------------------------------------------------*/
#include "external/speex_resampler.h"
/*---------------------------------------------------------------------------*/

#if RL_RESAMPLER_QUALITY < SPEEX_RESAMPLER_QUALITY_MIN || RL_RESAMPLER_QUALITY > SPEEX_RESAMPLER_QUALITY_MAX
#error Invalid quality value in RL_RESAMPLER_QUALITY
#endif

static char passthrough = 0;

rl_resampler_t* rl_resampler_create( unsigned in_freq )
{
  if ( in_freq != RL_SAMPLE_RATE )
  {
    return (rl_resampler_t*)speex_resampler_init( 2, in_freq, RL_SAMPLE_RATE, RL_RESAMPLER_QUALITY, NULL );
  }
  
  return (rl_resampler_t*)&passthrough;
}

void rl_resampler_destroy( rl_resampler_t* resampler )
{
  if ( resampler != (rl_resampler_t*)&passthrough )
  {
    speex_resampler_destroy( (SpeexResamplerState*)resampler );
  }
}

size_t rl_resampler_out_samples( rl_resampler_t* resampler, size_t in_samples )
{
  if ( resampler != (rl_resampler_t*)&passthrough )
  {
    SpeexResamplerState* st = (SpeexResamplerState*)resampler;
    spx_uint32_t in_rate, out_rate;
    
    speex_resampler_get_rate( st, &in_rate, &out_rate );
    return in_samples * out_rate / in_rate;
  }
  else
  {
    return in_samples;
  }
}

size_t rl_resampler_in_samples( rl_resampler_t* resampler, size_t out_samples )
{
  if ( resampler != (rl_resampler_t*)&passthrough )
  {
    SpeexResamplerState* st = (SpeexResamplerState*)resampler;
    spx_uint32_t in_rate, out_rate;
    
    speex_resampler_get_rate( st, &in_rate, &out_rate );
    return out_samples * in_rate / out_rate;
  }
  else
  {
    return out_samples;
  }
}

size_t rl_resampler_resample( rl_resampler_t* resampler, const int16_t* in_buffer, size_t in_samples, int16_t* out_buffer, size_t out_samples )
{
  if ( resampler != (rl_resampler_t*)&passthrough )
  {
    spx_uint32_t in_len = in_samples;
    spx_uint32_t out_len = out_samples;

    int error = speex_resampler_process_int( (SpeexResamplerState*)resampler, 0, in_buffer, &in_len, out_buffer, &out_len );
    return error == RESAMPLER_ERR_SUCCESS ? out_len : 0;
  }
  else
  {
    if ( in_samples > out_samples )
    {
      in_samples = out_samples;
    }
    
    memcpy( out_buffer, in_buffer, in_samples * sizeof( int16_t ) );
    return in_samples;
  }
}
