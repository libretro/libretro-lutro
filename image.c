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
      { "newImageData", img_newImageData },
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

int img_newImageData(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 1)
      return luaL_error(L, "lutro.image.newImageData requires 1 argument, %d given.", n);

   const char* path = luaL_checkstring(L, 1);

   char fullpath[PATH_MAX_LENGTH];
   strlcpy(fullpath, settings.gamedir, sizeof(fullpath));
   strlcat(fullpath, path, sizeof(fullpath));

   bitmap_t* self = (bitmap_t*)lua_newuserdata(L, sizeof(bitmap_t));

   rpng_load_image_argb(fullpath, &self->data, &self->width, &self->height);

   num_imgdatas++;
   imgdatas = (bitmap_t**)realloc(imgdatas, num_imgdatas * sizeof(bitmap_t));
   imgdatas[num_imgdatas-1] = self;

   if (luaL_newmetatable(L, "ImageData") != 0)
   {
      static luaL_Reg imgdata_funcs[] = {
         { "getWidth",   imgdata_getWidth },
         { "getHeight",  imgdata_getWidth },
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

   return 1;
}

int imgdata_getWidth(lua_State *L) 
{
   bitmap_t* self = (bitmap_t*)luaL_checkudata(L, 1, "Image");
   lua_pushnumber(L, self->width);
   return 1;
}

int imgdata_getHeight(lua_State *L) 
{
   bitmap_t* self = (bitmap_t*)luaL_checkudata(L, 1, "Image");
   lua_pushnumber(L, self->height);
   return 1;
}

int imgdata_getDimensions(lua_State *L) 
{
   bitmap_t* self = (bitmap_t*)luaL_checkudata(L, 1, "Image");
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
