#include "graphics.h"
#include "image.h"
#include "lutro.h"
#include "painter.h"
#include "compat/strl.h"
#include "lutro_stb_image.h"

#include <stdlib.h>
#include <string.h>

static int l_newImageData(lua_State *L);
static int l_getWidth(lua_State *L);
static int l_getHeight(lua_State *L);
static int l_getPixel(lua_State *L);
static int l_setPixel(lua_State *L);
static int l_getDimensions(lua_State *L);
static int l_type(lua_State *L);
static int l_gc(lua_State *L);

int lutro_image_preload(lua_State *L)
{
   static const luaL_Reg img_funcs[] =  {
      { "newImageData", l_newImageData },
      {NULL, NULL}
   };

   lutro_newlib(L, img_funcs, "image");

   return 1;
}

void lutro_image_init()
{
}

void *image_data_create(lua_State *L, bitmap_t* self)
{
   if (luaL_newmetatable(L, "ImageData") != 0)
   {
      static luaL_Reg imgdata_funcs[] = {
         { "getWidth",      l_getWidth },
         { "getHeight",     l_getHeight },
         { "getPixel",      l_getPixel },
         { "setPixel",      l_setPixel },
         { "getDimensions", l_getDimensions },
         { "type",          l_type },
         { "__gc",          l_gc },
         {NULL, NULL}
      };

      lua_pushvalue(L, -1);

      lua_setfield(L, -2, "__index");

      lua_pushcfunction(L, l_gc);
      lua_setfield( L, -2, "__gc" );

      luaL_setfuncs(L, imgdata_funcs, 0);
   }

   lua_setmetatable(L, -2);

   return self;
}

void *image_data_create_from_path(lua_State *L, const char *path)
{
   char fullpath[PATH_MAX_LENGTH];
   strlcpy(fullpath, settings.gamedir, sizeof(fullpath));
   strlcat(fullpath, path, sizeof(fullpath));

   bitmap_t* self = (bitmap_t*)lua_newuserdata(L, sizeof(bitmap_t));

   lutro_stb_image_load(fullpath, &self->data, &self->width, &self->height);

   self->pitch = self->width << 2;

   return image_data_create(L, self);
}

void *image_data_create_from_dimensions(lua_State *L, int width, int height)
{
   bitmap_t* self = (bitmap_t*)lua_newuserdata(L, sizeof(bitmap_t));

   self->width = width;
   self->height = height;
   self->pitch = self->width << 2;
   self->data = (uint32_t*)lutro_calloc(1, sizeof(uint32_t)*self->width*self->height);

   return image_data_create(L, self);
}

static int l_newImageData(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 1 && n != 2)
      return luaL_error(L, "lutro.image.newImageData requires 1 or 2 arguments, %d given.", n);

   if (n == 1)
   {
      const char* path = luaL_checkstring(L, 1);
      image_data_create_from_path(L, path);
   }
   else if (n == 2)
   {
      int width = luaL_checknumber(L, 1);
      int height = luaL_checknumber(L, 2);
      image_data_create_from_dimensions(L, width, height);
   }

   return 1;
}

static int l_type(lua_State *L)
{
   lua_pushstring(L, "ImageData");
   return 1;
}

static int l_getWidth(lua_State *L)
{
   bitmap_t* self = (bitmap_t*)luaL_checkudata(L, 1, "ImageData");
   lua_pushnumber(L, self->width);
   return 1;
}

static int l_getHeight(lua_State *L)
{
   bitmap_t* self = (bitmap_t*)luaL_checkudata(L, 1, "ImageData");
   lua_pushnumber(L, self->height);
   return 1;
}

static int l_getPixel(lua_State *L)
{
   int n = lua_gettop(L);

   bitmap_t* self = (bitmap_t*)luaL_checkudata(L, 1, "ImageData");

   int x = luaL_checknumber(L, 2);
   int y = luaL_checknumber(L, 3);

   lua_pop(L, n);

   int a = 0;
   int r = 0;
   int g = 0;
   int b = 0;

   if (self->data) {
       uint32_t* data = self->data;

       uint32_t color = data[y * (self->pitch >> 2) + x];

       a = ((color & ALPHA_MASK) >> ALPHA_SHIFT);
       r = ((color & RED_MASK) >> RED_SHIFT);
       g = ((color & GREEN_MASK) >> GREEN_SHIFT);
       b = ((color & BLUE_MASK) >> BLUE_SHIFT);
   }

   lua_pushnumber(L, r);
   lua_pushnumber(L, g);
   lua_pushnumber(L, b);
   lua_pushnumber(L, a);
   return 4;
}

static int l_setPixel(lua_State *L)
{
   int n = lua_gettop(L);

   bitmap_t* self = (bitmap_t*)luaL_checkudata(L, 1, "ImageData");

   gfx_Color c;

   int x = luaL_checknumber(L, 2);
   int y = luaL_checknumber(L, 3);
   c.r = luaL_checkint(L, 4);
   c.g = luaL_checkint(L, 5);
   c.b = luaL_checkint(L, 6);
   c.a = luaL_optint(L, 7, 255);

   lua_pop(L, n);

   if (self->data)
       self->data[y * (self->pitch >> 2) + x] = (c.a<<24) | (c.r<<16) | (c.g<<8) | c.b;

   return 0;
}

static int l_getDimensions(lua_State *L)
{
   bitmap_t* self = (bitmap_t*)luaL_checkudata(L, 1, "ImageData");
   lua_pushnumber(L, self->width);
   lua_pushnumber(L, self->height);
   return 2;
}

static int l_gc(lua_State *L)
{
   bitmap_t* self = (bitmap_t*)luaL_checkudata(L, 1, "ImageData");
   if (self->data) {
       lutro_free(self->data);
       self->data = NULL;
   }
   return 0;
}
