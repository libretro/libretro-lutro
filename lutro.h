#ifndef LUTRO_H
#define LUTRO_H

#include <stdint.h>
#include <stdbool.h>
#include <libretro.h>

#include "lutro_assert.h"

#ifndef PATH_MAX_LENGTH
#define PATH_MAX_LENGTH 4096
#endif

#define VERSION_MAJOR 0
#define VERSION_MINOR 0
#define VERSION_PATCH 1
#define VERSION_STRING "0.0.1"

typedef struct lutro_settings_t {
   int width;
   int height;
   int pitch;
   int pitch_pixels; // pitch in pixels to avoid recalculating it all the time
   uint32_t *framebuffer;
   retro_input_state_t input_cb;
   int live_enable;
   int live_call_load;
   char gamedir[PATH_MAX_LENGTH];
   char identity[PATH_MAX_LENGTH];
   double delta;
   double deltaCounter;
   int frameCounter;
   int fps;
   retro_environment_t* environ_cb;
} lutro_settings_t;

extern lutro_settings_t settings;
extern struct retro_perf_callback perf_cb;

void lutro_init();
void lutro_deinit();

int lutro_load(const char *path);
void lutro_run(double delta);
void lutro_reset();
size_t lutro_serialize_size();
bool lutro_serialize(void *data_, size_t size);
bool lutro_unserialize(const void *data_, size_t size);

void lutro_shutdown_game(void);

typedef struct _AssetPathInfo
{
   char fullpath[4096];
   char ext[16];     // ext is only for matching ones we know so its OK to truncate long ones.
} AssetPathInfo;

void lutro_assetPath_init(AssetPathInfo* dest, const char* path);

#endif // LUTRO_H
