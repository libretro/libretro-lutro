#ifndef RL_CONFIG_H
#define RL_CONFIG_H

/* retroluxury version. */
#define RL_VERSION_MAJOR 1
#define RL_VERSION_MINOR 0

/*
The margin to use when blitting sprites, must be a power of 2. The exact same
value must be used when rle-encoding images with rlrle.lua.
*/
#ifndef RL_BACKGRND_MARGIN
#define RL_BACKGRND_MARGIN 32
#endif

/* The maximum *total* number of sprites. */
#ifndef RL_MAX_SPRITES
#define RL_MAX_SPRITES 512
#endif

/* The maximum number of saved pixels when blitting sprites. */
#ifndef RL_BG_SAVE_SIZE
#define RL_BG_SAVE_SIZE ( 64 * 1024 )
#endif

/* The number of video frames per second. */
#ifndef RL_FRAME_RATE
#define RL_FRAME_RATE 60
#endif

/* The frequency of the mixed sound. */
#ifndef RL_SAMPLE_RATE
#define RL_SAMPLE_RATE 44100
#endif

/* Number of 16-bit stereo samples per frame. DO NOT CHANGE! */
#define RL_SAMPLES_PER_FRAME ( RL_SAMPLE_RATE / RL_FRAME_RATE )

/* The quality to use with the resampler [0, 10]. */
#define RL_RESAMPLER_QUALITY 4

/* The maximum number of simultaneous *playing* sounds. */
#ifndef RL_MAX_VOICES
#define RL_MAX_VOICES 8
#endif

/* The buffer increment to use when opening ogg files. */
#ifndef RL_OGG_INCREMENT
#define RL_OGG_INCREMENT 32768
#endif

/* Undef to remove ogg-vorbis support. */
#define RL_OGG_VORBIS

/* The number of userdata entries in other structures. */
#ifndef RL_USERDATA_COUNT
#define RL_USERDATA_COUNT 4
#endif

typedef struct
{
  unsigned version_major;
  unsigned version_minor;
  unsigned backgrnd_margin;
  unsigned max_sprites;
  unsigned bg_save_size;
  unsigned frame_rate;
  unsigned sample_rate;
  unsigned samples_per_frame;
  unsigned resampler_quality;
  unsigned max_voices;
  unsigned ogg_increment;
  unsigned ogg_vorbis; /* != 0 if compiled with ogg vorbis support */
  unsigned userdata_count;
}
rl_config_t;

/* Do *not* use the macros above, use this function to retrieve the runtime values. */
const rl_config_t* rl_get_config( void );

#endif /* RL_CONFIG_H */
