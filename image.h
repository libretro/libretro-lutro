#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "runtime.h"
#include "formats/rpng.h"

void lutro_image_init();
int lutro_image_preload(lua_State *L);

void *image_data_create(lua_State *L, const char *path);

#endif // IMAGE_H
