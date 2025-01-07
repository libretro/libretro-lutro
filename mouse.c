#include <stdlib.h>
#include <string.h>

#include <libretro.h>

#include "mouse.h"
#include "lutro.h"

static int16_t mouse_cache[8];

int lutro_mouse_preload(lua_State *L)
{
   static const luaL_Reg mouse_funcs[] =  {
      { "isDown", mouse_isDown },
      { "getX", mouse_getX },
      { "getY", mouse_getY },
      { "getPosition", mouse_getPosition },
      {NULL, NULL}
   };

   lutro_newlib(L, mouse_funcs, "mouse");

   return 1;
}

void lutro_mouse_init()
{
}

void lutro_mouseevent(lua_State* L)
{
   unsigned i;
   for (i = 0; i < 8; i++)
   {
      int16_t value = settings.input_cb(0, RETRO_DEVICE_MOUSE, 0, i);
      if (i == RETRO_DEVICE_ID_MOUSE_X || i == RETRO_DEVICE_ID_MOUSE_Y) {
        mouse_cache[i] += value;
      }
      else {
        mouse_cache[i] = value;
      }
   }
}

/**
 * lutro.mouse.isDown()
 *
 * https://love2d.org/wiki/love.mouse.isDown
 */
int mouse_isDown(lua_State *L)
{
   int n = lua_gettop(L);
   if (n < 1) {
      return luaL_error(L, "lutro.mouse.isDown requires 1 or more arguments, %d given.", n);
   }

   int buttonToCheck = 0;
   bool output = false;
   unsigned i;
   for (i = 0; i < n; i++) {
      buttonToCheck = (int) luaL_checknumber(L, i + 1);
      if (buttonToCheck == 1) {
         buttonToCheck = RETRO_DEVICE_ID_MOUSE_LEFT;
      }
      else if (buttonToCheck == 2) {
         buttonToCheck = RETRO_DEVICE_ID_MOUSE_RIGHT;
      }
      else if (buttonToCheck == 3) {
         buttonToCheck = RETRO_DEVICE_ID_MOUSE_MIDDLE;
      }
      else {
         buttonToCheck = 0;
      }
      if (buttonToCheck > 0) {
         if (mouse_cache[buttonToCheck]) {
            output = true;
            break;
         }
      }
   }
   lua_pushboolean(L, output);

   return 1;
}

/**
 * lutro.mouse.getX()
 *
 * https://love2d.org/wiki/love.mouse.getX
 */
int mouse_getX(lua_State *L)
{
   int n = lua_gettop(L);
   if (n > 0) {
      return luaL_error(L, "lutro.mouse.getX takes no arguments, %d given.", n);
   }

   unsigned x = (unsigned) mouse_cache[RETRO_DEVICE_ID_MOUSE_X];
   lua_pushnumber(L, x);

   return 1;
}

/**
 * lutro.mouse.getY()
 *
 * https://love2d.org/wiki/love.mouse.getY
 */
int mouse_getY(lua_State *L)
{
   int n = lua_gettop(L);
   if (n > 0) {
      return luaL_error(L, "lutro.mouse.getX takes no arguments, %d given.", n);
   }

   unsigned y = (unsigned) mouse_cache[RETRO_DEVICE_ID_MOUSE_Y];
   lua_pushnumber(L, y);

   return 1;
}

/**
 * lutro.mouse.getPosition()
 *
 * https://love2d.org/wiki/love.mouse.getPosition
 */
int mouse_getPosition(lua_State *L)
{
   int n = lua_gettop(L);
   if (n > 0) {
      return luaL_error(L, "lutro.mouse.getX takes no arguments, %d given.", n);
   }

   unsigned x = (unsigned) mouse_cache[RETRO_DEVICE_ID_MOUSE_X];
   unsigned y = (unsigned) mouse_cache[RETRO_DEVICE_ID_MOUSE_Y];
   lua_pushnumber(L, x);
   lua_pushnumber(L, y);

   return 2;
}

