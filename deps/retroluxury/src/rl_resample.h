#ifndef RL_RESAMPLE_H
#define RL_RESAMPLE_H

#include <rl_userdata.h>
#include <rl_imgdata.h>

#include <stdint.h>

typedef void rl_resampler_t;

/* Creates a resampler. */
rl_resampler_t* rl_resampler_create( unsigned in_freq );
/* Destroys a resampler. */
void            rl_resampler_destroy( rl_resampler_t* resampler );

/* Returns how much samples an output buffer must have to resample an input buffer with this number of samples. */
size_t rl_resampler_out_samples( rl_resampler_t* resampler, size_t in_samples );
/* Returns how much samples an input buffer must have to fill an output buffer with this number of samples. */
size_t rl_resampler_in_samples( rl_resampler_t* resampler, size_t out_samples );
/* Resamples the input buffer into the output buffer. */
size_t rl_resampler_resample( rl_resampler_t* resampler, const int16_t* in_buffer, size_t in_samples, int16_t* out_buffer, size_t out_samples );

#endif /* RL_RESAMPLE_H */
