#include <stdlib.h>
#include <string.h>

#include <libretro.h>

#include "mouse.h"
#include "lutro.h"

/**
 * The number of mouse IDs to track. Essentially RETRO_DEVICE_ID_MOUSE_MIDDLE + 1
 * 
 * @see RETRO_DEVICE_ID_MOUSE_MIDDLE
 */
#define MOUSE_CACHE_SIZE 7

/**
 * Current state of the mouse.
 */
static int16_t mouse_cache[MOUSE_CACHE_SIZE];

/**
 * The input device that will drive lutro.mouse events.
 *
 * @see RETRO_DEVICE_MOUSE
 * @see RETRO_DEVICE_POINTER
 */
static unsigned mouse_device = RETRO_DEVICE_MOUSE;

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

void lutro_mouse_init(void)
{
}

void lutro_mouse_setdevice(unsigned device)
{
   switch (device) {
      case RETRO_DEVICE_MOUSE:
      case RETRO_DEVICE_POINTER:
         mouse_device = device;
         break;
      default:
         device = RETRO_DEVICE_MOUSE;
   }
}

/**
 * Update the state of the mouse using the pointer device.
 */
static void lutro_pointerevent(void)
{
   int16_t px = settings.input_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
   int16_t py = settings.input_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);
   int16_t pressed = settings.input_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED);

   if (px != 0 || py != 0 || pressed)
   {
      int x = (int)(((int32_t)px + 0x7fff) * settings.width  / 0xfffe);
      int y = (int)(((int32_t)py + 0x7fff) * settings.height / 0xfffe);

      // Clamp the position.
      if (x < 0) x = 0;
      else if (x > settings.width - 1) x = settings.width - 1;
      if (y < 0) y = 0;
      else if (y > settings.height - 1) y = settings.height - 1;

      mouse_cache[RETRO_DEVICE_ID_MOUSE_X] = (int16_t)x;
      mouse_cache[RETRO_DEVICE_ID_MOUSE_Y] = (int16_t)y;
   }

   // Allow using mouse events for the Pointer.
   mouse_cache[RETRO_DEVICE_ID_MOUSE_LEFT] = pressed ||
      settings.input_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);
   mouse_cache[RETRO_DEVICE_ID_MOUSE_RIGHT] =
      settings.input_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT);
   mouse_cache[RETRO_DEVICE_ID_MOUSE_MIDDLE] =
      settings.input_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_MIDDLE);
   mouse_cache[RETRO_DEVICE_ID_MOUSE_WHEELUP] =
      settings.input_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELUP);
   mouse_cache[RETRO_DEVICE_ID_MOUSE_WHEELDOWN] =
      settings.input_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELDOWN);
}

static void lutro_relativeevent(void)
{
   unsigned i;
   for (i = 0; i < MOUSE_CACHE_SIZE; i++)
   {
      int16_t value = settings.input_cb(0, RETRO_DEVICE_MOUSE, 0, i);
      if (i == RETRO_DEVICE_ID_MOUSE_X || i == RETRO_DEVICE_ID_MOUSE_Y) {
         mouse_cache[i] += value;
      }
      else {
         mouse_cache[i] = value;
      }
   }

   // Clamp the mouse position.
   if (mouse_cache[RETRO_DEVICE_ID_MOUSE_X] < 0)
      mouse_cache[RETRO_DEVICE_ID_MOUSE_X] = 0;
   else if (mouse_cache[RETRO_DEVICE_ID_MOUSE_X] > settings.width - 1)
      mouse_cache[RETRO_DEVICE_ID_MOUSE_X] = settings.width - 1;
   if (mouse_cache[RETRO_DEVICE_ID_MOUSE_Y] < 0)
      mouse_cache[RETRO_DEVICE_ID_MOUSE_Y] = 0;
   else if (mouse_cache[RETRO_DEVICE_ID_MOUSE_Y] > settings.height - 1)
      mouse_cache[RETRO_DEVICE_ID_MOUSE_Y] = settings.height - 1;
}

void lutro_mouseevent(lua_State* L)
{
   if (mouse_device == RETRO_DEVICE_POINTER)
      lutro_pointerevent();
   else
      lutro_relativeevent();
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
      switch (buttonToCheck) {
         case 1: buttonToCheck = RETRO_DEVICE_ID_MOUSE_LEFT; break;
         case 2: buttonToCheck = RETRO_DEVICE_ID_MOUSE_RIGHT; break;
         case 3: buttonToCheck = RETRO_DEVICE_ID_MOUSE_MIDDLE; break;
         case 4: buttonToCheck = RETRO_DEVICE_ID_MOUSE_WHEELUP; break;
         case 5: buttonToCheck = RETRO_DEVICE_ID_MOUSE_WHEELDOWN; break;
         default: buttonToCheck = 0; break;
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
