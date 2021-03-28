#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "runtime.h"

void lutro_image_init();
int lutro_image_preload(lua_State *L);

void *image_data_create_from_path(lua_State *L, const char *path);

#endif // IMAGE_H
