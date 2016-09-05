#include <stdlib.h>
#include <libretro.h>

#include <rl_backgrnd.h>
#include <rl_imgdata.h>
#include <rl_image.h>
#include <rl_sprite.h>
#include <rl_snddata.h>
#include <rl_sound.h>

#include "res/smile.h"
#include "res/sketch008.h"
#include "res/bounce.h"

#define WIDTH  320
#define HEIGHT 240
#define COUNT  RL_MAX_VOICES

typedef struct
{
  rl_sprite_t* sprite;
  int dx, dy;
}
smile_t;

typedef struct
{
  rl_imgdata_t imgdata;
  rl_image_t   image;
  smile_t      smiles[ COUNT ];
  rl_voice_t*  music;
  rl_snddata_t snddata;
  rl_sound_t   sound;
}
state_t;

static void dummy_log( enum retro_log_level level, const char* fmt, ... )
{
  (void)level;
  (void)fmt;
}

static retro_video_refresh_t      video_cb;
static retro_input_poll_t         input_poll_cb;
static retro_environment_t        env_cb;
static retro_log_printf_t         log_cb = dummy_log;
static retro_audio_sample_batch_t audio_cb;

static state_t state;

void retro_get_system_info( struct retro_system_info* info )
{
  info->library_name = "retroluxury test";
  info->library_version = "0.1";
  info->need_fullpath = false;
  info->block_extract = false;
  info->valid_extensions = "";
}

void retro_set_environment( retro_environment_t cb )
{
  env_cb = cb;
  
  bool yes = true;
  cb( RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &yes );
}

unsigned retro_api_version( void )
{
  return RETRO_API_VERSION;
}

void retro_init( void )
{
  struct retro_log_callback log;

  if ( env_cb( RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log ) )
  {
    log_cb = log.log;
  }
}

bool retro_load_game( const struct retro_game_info* info )
{
  enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
  
  if ( !env_cb( RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt ) )
  {
    log_cb( RETRO_LOG_ERROR, "retroluxury needs RGB565\n" );
    return false;
  }
  
  rl_image_init();
  rl_sprite_init();
  
  if ( rl_backgrnd_create( WIDTH, HEIGHT, RL_BACKGRND_EXACT ) != 0 )
  {
    log_cb( RETRO_LOG_ERROR, "Error creating the framebuffer\n" );
    return false;
  }
  
  rl_backgrnd_clear( RL_COLOR( 64, 64, 64 ) );
  
  if ( rl_imgdata_create( &state.imgdata, res_smile_png, sizeof( res_smile_png ) ) != 0 )
  {
    log_cb( RETRO_LOG_ERROR, "Error loading image smile.png\n" );
error1:
    rl_backgrnd_destroy();
    return false;
  }
  
  if ( rl_image_create( &state.image, &state.imgdata, 0, 0 ) != 0 )
  {
    log_cb( RETRO_LOG_ERROR, "Error creating RLE-image\n" );
error2:
    rl_imgdata_destroy( &state.imgdata );
    goto error1;
  }
  
  for ( unsigned i = 0; i < COUNT; i++ )
  {
    rl_sprite_t* sprite = rl_sprite_create();
    
    if ( sprite == NULL )
    {
      log_cb( RETRO_LOG_ERROR, "Error creating sprite %u of %u\n", i, COUNT );
error3:
      rl_image_destroy( &state.image );
      
      for ( i = 0; i < COUNT; i++ )
      {
        if ( state.smiles[ i ].sprite != NULL )
        {
          rl_sprite_destroy( state.smiles[ i ].sprite );
        }
      }
      
      goto error2;
    }
    
    sprite->x = rand() % ( WIDTH - state.imgdata.width + 1 );
    sprite->y = rand() % ( HEIGHT - state.imgdata.height + 1 );
    sprite->layer = i;
    sprite->image = &state.image;
    
    state.smiles[ i ].sprite = sprite;
    state.smiles[ i ].dx = rand() & 1 ? -1 : 1;
    state.smiles[ i ].dy = rand() & 1 ? -1 : 1;
  }
  
  state.music = rl_sound_play_ogg( res_sketch008_ogg, sizeof( res_sketch008_ogg ), 1, NULL );
  
  if ( state.music == NULL )
  {
    log_cb( RETRO_LOG_ERROR, "Error loading music sketch008.ogg\n" );
    goto error3;
  }
  
  if ( rl_snddata_create( &state.snddata, res_bounce_wav, sizeof( res_bounce_wav ) ) != 0 )
  {
    log_cb( RETRO_LOG_ERROR, "Error loading wave bounce.wav\n" );
error4:
    rl_sound_stop( state.music );
    goto error3;
  }
  
  if ( rl_sound_create( &state.sound, &state.snddata ) != 0 )
  {
    log_cb( RETRO_LOG_ERROR, "Error creating PCM data\n" );
//error5:
    rl_snddata_destroy( &state.snddata );
    goto error4;
  }
  
  return true;
}

size_t retro_get_memory_size( unsigned id )
{
  return 0;
}

void* retro_get_memory_data( unsigned id )
{
  return NULL;
}

void retro_set_video_refresh( retro_video_refresh_t cb )
{
  video_cb = cb;
}

void retro_set_audio_sample( retro_audio_sample_t cb )
{
  (void)cb;
}

void retro_set_audio_sample_batch( retro_audio_sample_batch_t cb )
{
  audio_cb = cb;
}

void retro_set_input_state( retro_input_state_t cb )
{
  (void)cb;
}

void retro_set_input_poll( retro_input_poll_t cb )
{
  input_poll_cb = cb;
}

void retro_get_system_av_info( struct retro_system_av_info* info )
{
  const rl_config_t* config = rl_get_config();
  
  info->geometry.base_width = WIDTH + config->backgrnd_margin;
  info->geometry.base_height = HEIGHT;
  info->geometry.max_width = WIDTH + config->backgrnd_margin;
  info->geometry.max_height = HEIGHT;
  info->geometry.aspect_ratio = 0.0f;
  info->timing.fps = config->frame_rate;
  info->timing.sample_rate = config->sample_rate;
}

void retro_run( void )
{
  const rl_config_t* config = rl_get_config();
  
  input_poll_cb();
  
  unsigned width = WIDTH - state.imgdata.width;
  unsigned height = HEIGHT - state.imgdata.height;
  
  for ( unsigned i = 0; i < COUNT; i++ )
  {
    int bounced = 0;
    
    smile_t* smile = state.smiles + i;
    
    smile->sprite->x += smile->dx;
    smile->sprite->y += smile->dy;
    
    if ( smile->sprite->x < 0 || smile->sprite->x > width )
    {
      smile->dx = -smile->dx;
      smile->sprite->x += smile->dx;
      bounced = 1;
    }
    
    if ( smile->sprite->y < 0 || smile->sprite->y > height )
    {
      smile->dy = -smile->dy;
      smile->sprite->y += smile->dy;
      bounced = 1;
    }
    
    if ( bounced )
    {
      rl_sound_play( &state.sound, 0, NULL );
    }
  }
  
  rl_sprites_blit();
  video_cb( (void*)rl_backgrnd_fb( NULL, NULL ), WIDTH, HEIGHT, ( WIDTH + config->backgrnd_margin ) * 2 );
  rl_sprites_unblit();
  
  audio_cb( rl_sound_mix(), config->samples_per_frame );
}

void retro_deinit( void )
{
}

void retro_set_controller_port_device( unsigned port, unsigned device )
{
  (void)port;
  (void)device;
}

void retro_reset( void )
{
}

size_t retro_serialize_size( void )
{
  return 0;
}

bool retro_serialize( void* data, size_t size )
{
  return false;
}

bool retro_unserialize( const void* data, size_t size )
{
  return false;
}

void retro_cheat_reset( void )
{
}

void retro_cheat_set( unsigned a, bool b, const char* c )
{
  (void)a;
  (void)b;
  (void)c;
}

bool retro_load_game_special( unsigned a, const struct retro_game_info* b, size_t c )
{
  (void)a;
  (void)b;
  (void)c;
  return false;
}

void retro_unload_game( void )
{
  rl_sound_destroy( &state.sound );
  rl_snddata_destroy( &state.snddata );
  rl_sound_stop( state.music );
  
  for ( unsigned i = 0; i < COUNT; i++ )
  {
    rl_sprite_destroy( state.smiles[ i ].sprite );
  }
  
  rl_imgdata_destroy( &state.imgdata );
  rl_image_destroy( &state.image );
  rl_backgrnd_destroy();
}

unsigned retro_get_region( void )
{
  return RETRO_REGION_PAL;
}
