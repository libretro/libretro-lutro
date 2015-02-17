#ifndef LUTRO_H
#define LUTRO_H

#include <stdint.h>
#include "libretro.h"

#ifndef PATH_MAX_LENGTH
#define PATH_MAX_LENGTH 4096
#endif

typedef struct lutro_settings_t {
   int width;
   int height;
   int pitch;
   uint32_t *framebuffer;
   retro_input_state_t input_cb;
} lutro_settings_t;

extern lutro_settings_t settings;

void lutro_init();
void lutro_deinit();

int lutro_load(const char *path);

int lutro_run(double delta);

#endif // LUTRO_H
