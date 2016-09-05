#include <rl_config.h>

static const rl_config_t config =
{
  RL_VERSION_MAJOR,
  RL_VERSION_MINOR,
  RL_BACKGRND_MARGIN,
  RL_MAX_SPRITES,
  RL_BG_SAVE_SIZE,
  RL_FRAME_RATE,
  RL_SAMPLE_RATE,
  RL_SAMPLES_PER_FRAME,
  RL_RESAMPLER_QUALITY,
  RL_MAX_VOICES,
  RL_OGG_INCREMENT,
#ifdef RL_OGG_VORBIS
  1,
#else
  0,
#endif
  RL_USERDATA_COUNT
};

const rl_config_t* rl_get_config( void )
{
  return &config;
}
