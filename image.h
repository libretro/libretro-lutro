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

int img_newImageData(lua_State *L);

int imgdata_getWidth(lua_State *L);
int imgdata_getHeight(lua_State *L);
int imgdata_getPixel(lua_State *L);
int imgdata_getDimensions(lua_State *L);
int imgdata_type(lua_State *L);
int imgdata_gc(lua_State *L);

#endif // IMAGE_H
