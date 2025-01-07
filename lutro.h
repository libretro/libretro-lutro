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
void lutro_cheat_reset();
void lutro_cheat_set(unsigned index, bool enabled, const char *code);

void lutro_shutdown_game(void);

typedef struct _AssetPathInfo
{
   char fullpath[4096];
   char ext[16];     // ext is only for matching ones we know so its OK to truncate long ones.
} AssetPathInfo;

void lutro_assetPath_init(AssetPathInfo* dest, const char* path);

// Use wrapper to handle C allocation rather than direct call
// the goal is to easy allow to trace/check memory leak
#ifndef TRACE_ALLOCATION
#define TRACE_ALLOCATION 0 // Enable a printf trace of C allocation
#endif
void *lutro_malloc_internal(size_t size, const char* debug, int line);
void *lutro_calloc_internal(size_t nmemb, size_t size, const char* debug, int line);
void *lutro_realloc_internal(void *ptr, size_t size, const char* debug, int line);
void lutro_free_internal(void *ptr, const char* debug, int line);
void lutro_print_allocation();
#define lutro_malloc(size) lutro_malloc_internal(size, __FILE__, __LINE__)
#define lutro_calloc(nmemb, size) lutro_calloc_internal(nmemb, size, __FILE__, __LINE__)
#define lutro_realloc(ptr, size) lutro_realloc_internal(ptr, size, __FILE__, __LINE__)
#define lutro_free(ptr) lutro_free_internal(ptr, __FILE__, __LINE__)
#if TRACE_ALLOCATION // Allow to trace allocation done in stb (and freed in ludo)
#ifndef STBI_MALLOC
#define STBI_MALLOC(sz)           lutro_malloc_internal(sz, __FILE__, __LINE__)
#define STBI_REALLOC(p,newsz)     lutro_realloc_internal(p, newsz, __FILE__, __LINE__)
#define STBI_FREE(p)              lutro_free_internal(p, __FILE__, __LINE__)
#endif
#endif

#endif // LUTRO_H
