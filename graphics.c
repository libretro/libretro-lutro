#include "graphics.h"
#include "lutro.h"

#include <stdlib.h>
#include <string.h>

int lutro_graphics_preload(lua_State *L)
{
   static luaL_Reg gfx_funcs[] =  {
      { "clear",        gfx_clear },
      { "rectangle",    gfx_rectangle },
      { "newImage",     gfx_newImage },
      { "draw",         gfx_draw },
      { "drawq",        gfx_drawq },
      {NULL, NULL}
   };

   lutro_ensure_global_table(L, "lutro");

   luaL_newlib(L, gfx_funcs);

   lua_setfield(L, -2, "graphics");

   return 1;
}

void lutro_graphics_init()
{
   settings.pitch = settings.width * sizeof(uint32_t);
   settings.framebuffer = calloc(1, settings.pitch * settings.height*2);
}

int gfx_newImage(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 1)
      return luaL_error(L, "lutro.graphics.newImage requires 1 arguments, %i given.", n);

   const char* name = luaL_checkstring(L, 1);

   gfx_Image img;

   rpng_load_image_argb(name, &img.data, &img.width, &img.height);

   lua_newtable(L);

   lua_pushnumber(L, img.width);
   lua_setfield(L, -2, "width");

   lua_pushnumber(L, img.height);
   lua_setfield(L, -2, "height");

   lua_pushlightuserdata(L, img.data);
   lua_setfield(L, -2, "data");

   return 1;
}

int gfx_clear(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 1)
      return luaL_error(L, "lutro.graphics.clear requires 1 arguments, %i given.", n);

   uint32_t c = luaL_checkint(L, 1);

   lua_pop(L, n);

   int i, j;
   for (j = 0; j < settings.height; j++)
      for (i = 0; i < settings.width; i++)
         settings.framebuffer[j * (settings.pitch >> 1) + i] = c;

   return 0;
}

int gfx_rectangle(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 5)
      return luaL_error(L, "lutro.graphics.rectangle requires 5 arguments, %i given.", n);

   int x = luaL_checknumber(L, 1);
   int y = luaL_checknumber(L, 2);
   int w = luaL_checknumber(L, 3);
   int h = luaL_checknumber(L, 4);
   uint32_t c = luaL_checknumber(L, 5);

   lua_pop(L, n);

   int i, j;
   for (j = y; j < y + w; j++)
      for (i = x; i < x + h; i++)
         settings.framebuffer[j * (settings.pitch >> 1) + i] = c;

   return 0;
}

static void blit(int dest_x, int dest_y, int w, int h,
      int total_w, int total_h, uint32_t *data, int orig_x, int orig_y)
{
   int i, j;
   int jj = orig_y;
   int imgpitch = total_w * sizeof(uint16_t);
   for (j = dest_y; j < dest_y + h; j++) {
      int ii = orig_x;
      if (j >= 0 && j < settings.height) {
         for (i = dest_x; i < dest_x + w; i++) {
            if (i >= 0 && i < settings.width) {
               uint32_t c = data[jj * (imgpitch >> 1) + ii];
               if (0xff000000 & c)
                  settings.framebuffer[j * (settings.pitch >> 1) + i] = c;
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
   int orig_x = ((id-1)%(total_w/w))*w;
   int orig_y = ((id-1)/(total_w/w))*w;

   blit(dest_x, dest_y, w, h,
         total_w, total_h, data, orig_x, orig_y);
}

int gfx_drawq(lua_State *L)
{
   int camera_x = 0, camera_y = 0;
   int n = lua_gettop(L);

   if (n != 8)
      return luaL_error(L, "lutro.graphics.drawq requires 8 arguments, %i given.", n);

   int dest_x = luaL_checknumber(L, 1);
   int dest_y = luaL_checknumber(L, 2);
   int w = luaL_checknumber(L, 3);
   int h = luaL_checknumber(L, 4);
   int total_w = luaL_checknumber(L, 5);
   int total_h = luaL_checknumber(L, 6);
   uint32_t* data = lua_touserdata(L, 7);
   int id = luaL_checknumber(L, 8);

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
      w, h, total_w, total_h, data, id);

   return 1;
}

int gfx_draw(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 5)
      return luaL_error(L, "lutro.graphics.draw requires 5 arguments, %i given.", n);

   uint32_t* data = lua_touserdata(L, 1);
   int x = luaL_checknumber(L, 2);
   int y = luaL_checknumber(L, 3);
   int w = luaL_checknumber(L, 4);
   int h = luaL_checknumber(L, 5);

   lua_pop(L, n);

   blit(x, y, w, h, w, h, data, 0, 0);

   return 1;
}
