#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdlib.h>
#include "runtime.h"
#include "formats/rpng.h"

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

//typedef struct
//{
//   bitmap_t image;
//   int *separators;
//   const char *characters;
//} gfx_Font;

void lutro_graphics_init();
int lutro_graphics_preload(lua_State *L);

int gfx_clear(lua_State *L);
int gfx_point(lua_State *L);
int gfx_line(lua_State *L);
int gfx_rectangle(lua_State *L);
int gfx_newImage(lua_State *L);
int gfx_newImageFont(lua_State *L);
int gfx_newQuad(lua_State *L);
int gfx_setFont(lua_State *L);
int gfx_getFont(lua_State *L);
int gfx_setColor(lua_State *L);
int gfx_getColor(lua_State *L);
int gfx_setBackgroundColor(lua_State *L);
int gfx_getBackgroundColor(lua_State *L);
int gfx_draw(lua_State *L);
int gfx_drawt(lua_State *L);
int gfx_print(lua_State *L);
int gfx_setDefaultFilter(lua_State *L);
int gfx_setLineStyle(lua_State *L);
int gfx_setLineWidth(lua_State *L);
int gfx_scale(lua_State *L);
int gfx_getWidth(lua_State *L);
int gfx_getHeight(lua_State *L);

int img_getData(lua_State *L);
int img_getWidth(lua_State *L);
int img_getHeight(lua_State *L);
int img_getDimensions(lua_State *L);
int img_setFilter(lua_State *L);
int img_gc(lua_State *L);

int quad_getViewport(lua_State *L);
int quad_setViewport(lua_State *L);
int quad_gc(lua_State *L);

#endif // GRAPHICS_H
