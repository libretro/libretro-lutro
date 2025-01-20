#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "runtime.h"

#if defined(ABGR)
#define ALPHA_SHIFT 24
#define RED_SHIFT 0
#define GREEN_SHIFT 8
#define BLUE_SHIFT 16

#define ALPHA_MASK 0xFF000000
#define RED_MASK 0x000000FF
#define GREEN_MASK 0x0000FF00
#define BLUE_MASK 0x00FF0000
#else
#define ALPHA_SHIFT 24
#define RED_SHIFT 16
#define GREEN_SHIFT 8
#define BLUE_SHIFT 0

#define ALPHA_MASK 0xFF000000
#define RED_MASK 0x00FF0000
#define GREEN_MASK 0x0000FF00
#define BLUE_MASK 0x000000FF
#endif

void lutro_image_init(void);
int lutro_image_preload(lua_State *L);

void *image_data_create_from_path(lua_State *L, const char *path);

#endif // IMAGE_H
