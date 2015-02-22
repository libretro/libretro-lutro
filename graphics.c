#include "graphics.h"
#include "lutro.h"

#include <stdlib.h>
#include <string.h>

static gfx_Font *current_font;
static uint32_t current_color;

int lutro_graphics_preload(lua_State *L)
{
   static luaL_Reg gfx_funcs[] =  {
      { "clear",        gfx_clear },
      { "point",        gfx_point },
      { "line",         gfx_line },
      { "rectangle",    gfx_rectangle },
      { "newImage",     gfx_newImage },
      { "newImageFont", gfx_newImageFont },
      { "setFont",      gfx_setFont },
      { "getFont",      gfx_getFont },
      { "setColor",     gfx_setColor },
      { "getColor",     gfx_getColor },
      { "draw",         gfx_draw },
      { "drawq",        gfx_drawq },
      { "print",        gfx_print },
      {NULL, NULL}
   };

   lutro_ensure_global_table(L, "lutro");

   luaL_newlib(L, gfx_funcs);

   lua_setfield(L, -2, "graphics");

   return 1;
}

void lutro_graphics_init()
{
   // TODO: power of two framebuffers
   settings.pitch_pixels = settings.width;
   settings.pitch        = settings.pitch_pixels * sizeof(uint32_t);
   settings.framebuffer  = calloc(1, settings.pitch * settings.height);
   current_color = 0xffffffff;
}

int gfx_newImage(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 1)
      return luaL_error(L, "lutro.graphics.newImage requires 1 arguments, %d given.", n);

   const char* name = luaL_checkstring(L, 1);

   gfx_Image* self = (gfx_Image*)lua_newuserdata(L, sizeof(gfx_Image));

   rpng_load_image_argb(name, &self->data, &self->width, &self->height);

   if (luaL_newmetatable(L, "Image") != 0)
   {
      static luaL_Reg img_funcs[] = {
         { "getData",       img_getData },
         { "getWidth",      img_getWidth },
         { "getHeight",     img_getHeight },
         { "getDimensions", img_getDimensions },
         { "__gc",          img_gc },
         {NULL, NULL}
      };

      lua_pushvalue(L, -1);

      lua_setfield(L, -2, "__index");

      lua_pushcfunction( L, img_gc );
      lua_setfield( L, -2, "__gc" );

      luaL_setfuncs(L, img_funcs, 0);
   }

   lua_setmetatable(L, -2);

   return 1;
}

int img_getData(lua_State *L)
{
   gfx_Image* self = (gfx_Image*)luaL_checkudata(L, 1, "Image");
   lua_pushlightuserdata(L, self->data);
   return 1;
}

int img_getWidth(lua_State *L)
{
   gfx_Image* self = (gfx_Image*)luaL_checkudata(L, 1, "Image");
   lua_pushnumber(L, self->width);
   return 1;
}

int img_getHeight(lua_State *L)
{
   gfx_Image* self = (gfx_Image*)luaL_checkudata(L, 1, "Image");
   lua_pushnumber(L, self->height);
   return 1;
}

int img_getDimensions(lua_State *L)
{
   gfx_Image* self = (gfx_Image*)luaL_checkudata(L, 1, "Image");
   lua_pushnumber(L, self->width);
   lua_pushnumber(L, self->height);
   return 2;
}

int img_gc(lua_State *L)
{
   gfx_Image* self = (gfx_Image*)luaL_checkudata(L, 1, "Image");
   (void)self;
   return 0;
}

int gfx_newImageFont(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 2)
      return luaL_error(L, "lutro.graphics.newImageFont requires 2 arguments, %d given.", n);

   const char* imgpath = luaL_checkstring(L, 1);
   const char* characters = luaL_checkstring(L, 2);

   lua_pop(L, n);

   gfx_Image img;

   rpng_load_image_argb(imgpath, &img.data, &img.width, &img.height);

   uint32_t separator = img.data[0];

   int max_separators = strlen(characters);
   int *separators = calloc(max_separators, sizeof(int));

   int i, char_counter = 0;
   for (i = 0; i < img.width && char_counter < max_separators; i++)
   {
      uint32_t c = img.data[i];
      if (c == separator)
         separators[char_counter++] = i;
   }

   gfx_Font *font = calloc(1, sizeof(gfx_Font));
   font->image = img;
   font->separators = separators;
   font->characters = characters;

   lua_pushlightuserdata(L, font);

   return 1;
}

int gfx_setFont(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 1)
      return luaL_error(L, "lutro.graphics.setFont requires 1 arguments, %d given.", n);

   gfx_Font *font = lua_touserdata(L, 1);

   lua_pop(L, n);

   current_font = font;

   return 0;
}

int gfx_getFont(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 0)
      return luaL_error(L, "lutro.graphics.getFont requires 0 arguments, %d given.", n);

   lua_pop(L, n);

   lua_pushlightuserdata(L, current_font);

   return 1;
}

int gfx_setColor(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 4)
      return luaL_error(L, "lutro.graphics.setColor requires 4 arguments, %d given.", n);

   gfx_Color c;
   c.r = luaL_checkint(L, 1);
   c.g = luaL_checkint(L, 2);
   c.b = luaL_checkint(L, 3);
   c.a = luaL_checkint(L, 4);

   lua_pop(L, n);

   current_color = (c.a<<24) | (c.r<<16) | (c.g<<8) | c.b;

   return 0;
}

int gfx_getColor(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 0)
      return luaL_error(L, "lutro.graphics.getColor requires 0 arguments, %d given.", n);

   lua_pop(L, n);

   gfx_Color c;
   c.a = (current_color >> 24) & 0xff;
   c.r = (current_color >> 16) & 0xff;
   c.g = (current_color >>  8) & 0xff;
   c.b = (current_color >>  0) & 0xff;

   lua_pushnumber(L, c.r);
   lua_pushnumber(L, c.g);
   lua_pushnumber(L, c.b);
   lua_pushnumber(L, c.a);

   return 4;
}

int gfx_clear(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 1)
      return luaL_error(L, "lutro.graphics.clear requires 1 arguments, %d given.", n);

   uint32_t c = luaL_checkint(L, 1);

   lua_pop(L, n);

   unsigned i;
   unsigned size = settings.pitch_pixels * settings.height;
   uint32_t *framebuffer = settings.framebuffer;

   for (i = 0; i < size; ++i)
      framebuffer[i] = c;

   return 0;
}

int gfx_rectangle(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 4)
      return luaL_error(L, "lutro.graphics.rectangle requires 4 arguments, %d given.", n);

   int x = luaL_checknumber(L, 1);
   int y = luaL_checknumber(L, 2);
   int w = luaL_checknumber(L, 3);
   int h = luaL_checknumber(L, 4);

   lua_pop(L, n);

   int i, j;
   int pitch_pixels = settings.pitch_pixels;
   uint32_t *framebuffer = settings.framebuffer;
   for (j = y; j < y + w; j++)
      for (i = x; i < x + h; i++)
         framebuffer[j * pitch_pixels + i] = current_color;

   return 0;
}

int gfx_point(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 2)
      return luaL_error(L, "lutro.graphics.point requires 2 arguments, %d given.", n);

   int x = luaL_checknumber(L, 1);
   int y = luaL_checknumber(L, 2);

   lua_pop(L, n);

   int pitch_pixels = settings.pitch_pixels;
   uint32_t *framebuffer = settings.framebuffer;

   framebuffer[y * pitch_pixels + x] = current_color;

   return 0;
}

int gfx_line(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 4)
      return luaL_error(L, "lutro.graphics.line requires 4 arguments, %d given.", n);

   int x1 = luaL_checknumber(L, 1);
   int y1 = luaL_checknumber(L, 2);
   int x2 = luaL_checknumber(L, 3);
   int y2 = luaL_checknumber(L, 4);

   lua_pop(L, n);

   int pitch_pixels = settings.pitch_pixels;
   uint32_t *framebuffer = settings.framebuffer;

   int dx = abs(x2-x1), sx = x1<x2 ? 1 : -1;
   int dy = abs(y2-y1), sy = y1<y2 ? 1 : -1;
   int err = (dx>dy ? dx : -dy)/2, e2;

   for (;;) {
      if (y1 >= 0 && y1 < settings.height)
         if (x1 >= 0 && x1 < settings.width)
            framebuffer[y1 * pitch_pixels + x1] = current_color;
      if (x1==x2 && y1==y2) break;
      e2 = err;
      if (e2 >-dx) { err -= dy; x1 += sx; }
      if (e2 < dy) { err += dx; y1 += sy; }
   }

   return 0;
}

static void blit(int dest_x, int dest_y, int w, int h,
      int total_w, int total_h, uint32_t *data, int orig_x, int orig_y)
{
   int i, j;
   int jj = orig_y;
   int imgpitch = total_w * sizeof(uint16_t);

   int pitch_pixels = settings.pitch_pixels;
   uint32_t *framebuffer = settings.framebuffer;

   for (j = dest_y; j < dest_y + h; j++) {
      int ii = orig_x;
      if (j >= 0 && j < settings.height) {
         for (i = dest_x; i < dest_x + w; i++) {
            if (i >= 0 && i < settings.width) {
               uint32_t c = data[jj * (imgpitch >> 1) + ii];
               if (0xff000000 & c)
                  framebuffer[j * pitch_pixels + i] = c;
            }
            ii++;
         }
      }
      jj++;
   }
}

static void drawq(int dest_x, int dest_y, int w, int h,
      int total_w, int total_h, uint32_t *data, int id)
{
   if (dest_x + w < 0 || dest_y + h < 0
   || dest_x > settings.width || dest_y > settings.height)
      return;

   int orig_x = ((id-1)%(total_w/w))*w;
   int orig_y = ((id-1)/(total_w/w))*w;

   blit(dest_x, dest_y, w, h,
         total_w, total_h, data, orig_x, orig_y);
}

int gfx_drawq(lua_State *L)
{
   int camera_x = 0, camera_y = 0;
   int n = lua_gettop(L);

   if (n != 6)
      return luaL_error(L, "lutro.graphics.drawq requires 6 arguments, %d given.", n);

   gfx_Image* img = (gfx_Image*)luaL_checkudata(L, 1, "Image");
   int dest_x = luaL_checknumber(L, 2);
   int dest_y = luaL_checknumber(L, 3);
   int w = luaL_checknumber(L, 4);
   int h = luaL_checknumber(L, 5);
   int id = luaL_checknumber(L, 6);

   lua_pop(L, n);

   lua_getglobal(L, "lutro");

   lua_getfield(L, -1, "camera_x");
   camera_x = lua_tointeger(L, -1);
   lua_remove(L, -1);

   lua_getfield(L, -1, "camera_y");
   camera_y = lua_tointeger(L, -1);
   lua_remove(L, -1);

   drawq(
      dest_x + camera_x,
      dest_y + camera_y,
      w, h, img->width, img->height, img->data, id);

   return 0;
}

int gfx_draw(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 3)
      return luaL_error(L, "lutro.graphics.draw requires 3 arguments, %d given.", n);

   gfx_Image* img = (gfx_Image*)luaL_checkudata(L, 1, "Image");
   int x = luaL_checknumber(L, 2);
   int y = luaL_checknumber(L, 3);

   lua_pop(L, n);

   blit(x, y,
      img->width, img->height,
      img->width, img->height,
      img->data, 0, 0);

   return 0;
}

static int strpos(const char *haystack, char needle)
{
   char *p = strchr(haystack, needle);
   if (p)
      return p - haystack;
   return -1;
}

int gfx_print(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 3)
      return luaL_error(L, "lutro.graphics.print requires 3 arguments, %d given.", n);

   if (current_font == NULL)
      return luaL_error(L, "lutro.graphics.print requires a font to be set.");

   gfx_Font *font = current_font;
   const char* message = luaL_checkstring(L, 1);
   int dest_x = luaL_checknumber(L, 2);
   int dest_y = luaL_checknumber(L, 3);

   lua_pop(L, n);

   int i;
   for (i = 0; i < strlen(message); i++)
   {
      char c = message[i];

      int pos = strpos(font->characters, c);

      int orig_x = font->separators[pos] + 1;
      int w = font->separators[pos+1] - orig_x;
      int h = font->image.height;
      int total_w = font->image.width;
      int total_h = font->image.height;

      blit(dest_x, dest_y, w, h, total_w, total_h, font->image.data, orig_x, 0);

      dest_x += w + 1;
   }

   return 0;
}
