#ifndef RL_SNDDATA_H
#define RL_SNDDATA_H

#include <rl_userdata.h>

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct
{
  rl_userdata_t ud;
  
  int bps;      /* bits per sample */
  int channels; /* number of channels */
  int freq;     /* frequency */
  
  size_t size;
  const void* samples;
}
rl_snddata_t;

int     rl_snddata_create( rl_snddata_t* snddata, const void* data, size_t size );
#define rl_snddata_destroy( snddata ) do { free( (void*)( snddata )->samples ); } while ( 0 )

const int16_t* rl_snddata_encode( size_t* out_samples, const rl_snddata_t* snddata );

#endif /* RL_SNDDATA_H */
