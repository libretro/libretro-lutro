#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdlib.h>
#include "runtime.h"
#include "formats/rpng.h"
#include "painter.h"

typedef struct
{
   bitmap_t* data;
   int ref;
} gfx_Image;

typedef struct
{
   unsigned x;
   unsigned y;
   unsigned w;
   unsigned h;
   unsigned sw;
   unsigned sh;
} gfx_Quad;

typedef struct
{
   int r;
   int g;
   int b;
   int a;
} gfx_Color;

void lutro_graphics_init();
int lutro_graphics_preload(lua_State *L);

void lutro_graphics_reinit();
void lutro_graphics_begin_frame(lua_State *L);
void lutro_graphics_end_frame(lua_State *L);

#endif // GRAPHICS_H
