#include "image.h"
#include "lutro.h"
#include "painter.h"
#include "compat/strl.h"

#include <stdlib.h>
#include <string.h>

static unsigned num_imgdatas = 0;
static bitmap_t** imgdatas = NULL;

int lutro_image_preload(lua_State *L)
{
   static luaL_Reg img_funcs[] =  {
      { "newImageData", l_img_newImageData },
      {NULL, NULL}
   };

   lutro_ensure_global_table(L, "lutro");

   luaL_newlib(L, img_funcs);

   lua_setfield(L, -2, "image");

   return 1;
}

void lutro_image_init()
{
}

void *img_newImageData(lua_State *L, const char *path)
{
   char fullpath[PATH_MAX_LENGTH];
   strlcpy(fullpath, settings.gamedir, sizeof(fullpath));
   strlcat(fullpath, path, sizeof(fullpath));

   bitmap_t* self = (bitmap_t*)lua_newuserdata(L, sizeof(bitmap_t));

   rpng_load_image_argb(fullpath, &self->data, &self->width, &self->height);

   self->pitch = self->width << 2;

   num_imgdatas++;
   imgdatas = (bitmap_t**)realloc(imgdatas, num_imgdatas * sizeof(bitmap_t));
   imgdatas[num_imgdatas-1] = self;

   if (luaL_newmetatable(L, "ImageData") != 0)
   {
      static luaL_Reg imgdata_funcs[] = {
         { "getWidth",   imgdata_getWidth },
         { "getHeight",  imgdata_getHeight },
         { "getPixel",   imgdata_getPixel },
         { "type",       imgdata_type },
         { "__gc",       imgdata_gc },
         {NULL, NULL}
      };

      lua_pushvalue(L, -1);

      lua_setfield(L, -2, "__index");

      lua_pushcfunction(L, imgdata_gc);
      lua_setfield( L, -2, "__gc" );

      luaL_setfuncs(L, imgdata_funcs, 0);
   }

   lua_setmetatable(L, -2);

   return self;
}

int l_img_newImageData(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 1)
      return luaL_error(L, "lutro.image.newImageData requires 1 argument, %d given.", n);

   const char* path = luaL_checkstring(L, 1);

   img_newImageData(L, path);

   return 1;
}

int imgdata_type(lua_State *L)
{
   lua_pushstring(L, "ImageData");
   return 1;
}

int imgdata_getWidth(lua_State *L) 
{
   bitmap_t* self = (bitmap_t*)luaL_checkudata(L, 1, "ImageData");
   lua_pushnumber(L, self->width);
   return 1;
}

int imgdata_getHeight(lua_State *L) 
{
   bitmap_t* self = (bitmap_t*)luaL_checkudata(L, 1, "ImageData");
   lua_pushnumber(L, self->height);
   return 1;
}

int imgdata_getPixel(lua_State *L) 
{
   int n = lua_gettop(L);

   bitmap_t* self = (bitmap_t*)luaL_checkudata(L, 1, "ImageData");

   int x = luaL_checknumber(L, 2);
   int y = luaL_checknumber(L, 3);

   lua_pop(L, n);

   uint32_t* data = self->data;

   uint32_t color = data[y * (self->pitch >> 2) + x];

   int a = ((color & 0xff000000)>>24);
   int r = ((color & 0xff0000)>>16);
   int g = ((color & 0xff00)>>8);
   int b = (color & 0xff);

   lua_pushnumber(L, r);
   lua_pushnumber(L, g);
   lua_pushnumber(L, b);
   lua_pushnumber(L, a);
   return 4;
}

int imgdata_getDimensions(lua_State *L) 
{
   bitmap_t* self = (bitmap_t*)luaL_checkudata(L, 1, "ImageData");
   lua_pushnumber(L, self->width);
   lua_pushnumber(L, self->height);
   return 2;
}

int imgdata_gc(lua_State *L)
{
   bitmap_t* self = (bitmap_t*)luaL_checkudata(L, 1, "ImageData");
   (void)self;
   return 0;
}
