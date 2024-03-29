#include "lutro.h"
#include "audio.h"

#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <libretro.h>
#include <streams/file_stream.h>

#include "joystick.h"

int16_t audio_buffer[2 * AUDIO_FRAMES];

static struct retro_log_callback logging;

static retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

double frame_time = 0;

static void emit_audio(void)
{
   lutro_mixer_render(audio_buffer);
   audio_batch_cb(audio_buffer, AUDIO_FRAMES);
}

static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{
   (void)level;
   va_list va;
   va_start(va, fmt);
   vfprintf(stderr, fmt, va);
   va_end(va);
}

void retro_init(void)
{
   lutro_init();

   if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
      log_cb = logging.log;
   else
      log_cb = fallback_log;

   // Always get the perf interface because we need it for the timers
   if (!environ_cb( RETRO_ENVIRONMENT_GET_PERF_INTERFACE, &perf_cb))
   {
      perf_cb.get_time_usec = NULL;
      log_cb(RETRO_LOG_WARN, "Could not get the perf interface\n");
   }
}

void retro_deinit(void)
{
   lutro_deinit();
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   (void)port;
   (void)device;
}

/*---------------------------------------------------------------------------*/
/* compatibility functions */

#ifdef __CELLOS_LV2__

char* getenv( const char* name)
{
  (void)name;
  return NULL;
}

#endif

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name     = "lutro";
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
   info->library_version  = VERSION_STRING GIT_VERSION;
   info->need_fullpath    = true;
   info->valid_extensions = "lutro|lua";
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   info->timing.fps = 60.0;
   info->timing.sample_rate = 44100.0;

   info->geometry.base_width   = settings.width;
   info->geometry.base_height  = settings.height;
   info->geometry.max_width    = settings.width;
   info->geometry.max_height   = settings.height;
   info->geometry.aspect_ratio = (float)settings.width/(float)settings.height;
}

void retro_set_environment(retro_environment_t cb)
{
   struct retro_vfs_interface_info vfs_iface_info = { 1, NULL };
   environ_cb = cb;
   settings.environ_cb = &environ_cb;

   bool no_rom = false;
   cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_rom);

   if (environ_cb(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs_iface_info))
      filestream_vfs_init(&vfs_iface_info);
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
   settings.input_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

void retro_reset(void)
{
   lutro_reset();
}

static void frame_time_cb(retro_usec_t usec)
{
   frame_time = usec / 1000000.0;
}

void retro_run(void)
{
   input_poll_cb();

   lutro_run(frame_time);
   video_cb(settings.framebuffer, settings.width, settings.height, settings.pitch);
   emit_audio();
}

bool retro_load_game(const struct retro_game_info *info)
{
   struct retro_input_descriptor desc[] = {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "B" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "A" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "X" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "Y" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L, "L1" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R, "R1" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2, "L2" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2, "R2" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3, "L3" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3, "R3" },
      { 0 },
   };

   if (!info)
      return false;

   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
   {
      log_cb(RETRO_LOG_INFO, "XRGB_8888 is not supported.\n");
      return false;
   }

   struct retro_frame_time_callback frame_cb = { frame_time_cb, 1000000 / 60 };
   environ_cb(RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK, &frame_cb);

   if (!perf_cb.get_time_usec)
   {
      log_cb(RETRO_LOG_ERROR, "Core needs the perf interface\n");
      return false;
   }

   int success = lutro_load(info->path);

   return success;
}

void retro_unload_game(void)
{
   // Workaround a crash on Windows & Android because the callbacks are invoked after the DLL/SO was unloaded 
   struct retro_audio_callback no_audio_callback_definition = { NULL, NULL }; 
   environ_cb(RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK, &no_audio_callback_definition); 
}

void lutro_shutdown_game(void)
{
   environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, 0);
}

unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
   (void)type;
   (void)info;
   (void)num;
   return false;
}

size_t retro_serialize_size(void)
{
   return lutro_serialize_size();
}

bool retro_serialize(void *data_, size_t size)
{
   return lutro_serialize(data_, size);
}

bool retro_unserialize(const void *data_, size_t size)
{
   return lutro_unserialize(data_, size);
}

void *retro_get_memory_data(unsigned id)
{
//   if (id != RETRO_MEMORY_SAVE_RAM)
//      return NULL;

//   return game_data();
   return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
//   if (id != RETRO_MEMORY_SAVE_RAM)
//      return 0;

//   return game_data_size();
   return 0;
}

void retro_cheat_reset(void)
{
    lutro_cheat_reset();
}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   lutro_cheat_set(index, enabled, code);
}

#if defined(__QNX__) || defined(_MSC_VER)
/* QNX doesn't have this */
int vasprintf(char **strp, const char *fmt, va_list ap)
{
   /* measure the string */
   int len = vsnprintf(NULL,0,fmt,ap);
   /* try allocating some memory */
   if((len < 0) || ((*strp = lutro_malloc(++len)) == NULL)) return -1;
   /* print the string */
   len = vsnprintf(*strp,len,fmt,ap);
   /* handle failure */
   if(len < 0) lutro_free(*strp);
   return len;
}
#endif
