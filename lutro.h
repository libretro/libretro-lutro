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

extern int g_lua_stack;
#define ENTER_LUA_STACK do { g_lua_stack = lua_gettop(L); } while(0);
#define EXIT_LUA_STACK do { \
	int stack = lua_gettop(L); \
	if (stack != g_lua_stack) { \
		printf("invalid stack setup (got %d expected %d) on %s\n", stack, g_lua_stack, __func__); \
		lua_settop(L, g_lua_stack); \
	} \
} while (0);

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
// Note: use a debug string to easily identify the allocation. __FILE__
// + __LINE__ could be used too but it might be too verbose
#ifndef TRACE_ALLOCATION
#define TRACE_ALLOCATION 0 // Enable a printf trace of C allocation
#endif
void *lutro_malloc(size_t size, const char* debug);
void lutro_free(void *ptr, const char* debug);
void *lutro_calloc(size_t nmemb, size_t size, const char* debug);
void *lutro_realloc(void *ptr, size_t size, const char* debug);

#endif // LUTRO_H
