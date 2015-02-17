#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdlib.h>
#include "runtime.h"
#include "formats/rpng.h"

typedef struct
{
   uint32_t *data;
   unsigned width;
   unsigned height;
} gfx_Image;

void lutro_graphics_init();
int lutro_graphics_preload(lua_State *L);

int gfx_clear(lua_State *L);
int gfx_rectangle(lua_State *L);
int gfx_newImage(lua_State *L);
int gfx_draw(lua_State *L);
int gfx_drawq(lua_State *L);

#endif // GRAPHICS_H
