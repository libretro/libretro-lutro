#include <stdlib.h>
#include <string.h>

#include <libretro.h>

#include "lutro_window.h"
#include "lutro.h"
#include "graphics.h"

int lutro_window_preload(lua_State *L)
{
   static const luaL_Reg win_funcs[] =  {
      { "close", win_close },
      { "setTitle", win_setTitle },
      { "setMode",  win_setMode },
      { "setIcon", win_setIcon },
      { "isCreated", win_isCreated },
      { "maximize", win_maximize },
      { "minimize", win_minimize },
      { "getTitle", win_getTitle },
      { "setPosition", win_setPosition },
      { "getPosition", win_getPosition },
      { "requestAttention", win_requestAttention },
      { "getDisplayName", win_getDisplayName },
      { "setDisplaySleepEnabled", win_setDisplaySleepEnabled },
      { "isDisplaySleepEnabled", win_isDisplaySleepEnabled },
      { "showMessageBox", win_showMessageBox },
      {NULL, NULL}
   };

   lutro_newlib(L, win_funcs, "window");

   return 1;
}

void lutro_window_init()
{
}

/**
 * lutro.window.setTitle
 *
 * https://love2d.org/wiki/love.window.setTitle
 */
int win_setTitle(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 1)
      return luaL_error(L, "lutro.window.setTitle expects 1 arguments, %d given.", n);

   // Setting the title is ignored in Lutro.
   return 0;
}

/**
 * lutro.window.close()
 *
 * https://love2d.org/wiki/love.window.close
 */
int win_close(lua_State *L)
{
   lutro_shutdown_game();

   return 0;
}

/**
 * lutro.window.setIcon
 *
 * https://love2d.org/wiki/love.window.setIcon
 */
int win_setIcon(lua_State *L)
{
   // Pretend that we successfully set the window icon.
   lua_pushboolean(L, true);

   return 1;
}

int win_setMode(lua_State *L)
{
   int n = lua_gettop(L);

   if (n < 2)
      return luaL_error(L, "lutro.window.setMode at least 2 arguments, %d given.", n);

   settings.width = luaL_checknumber(L, 1);
   settings.height = luaL_checknumber(L, 2);

   lutro_graphics_reinit(L);

   lua_pop(L, n);

   lua_pushboolean(L, true);

   return 1;
}

/**
 * lutro.window.isCreated
 *
 * https://love2d.org/wiki/love.window.isCreated
 */
int win_isCreated(lua_State *L)
{
   int n = lua_gettop(L);

   if (n > 0)
      return luaL_error(L, "lutro.window.isCreated expects 0 arguments, %d given.", n);

   // The window will always be created in libretro.
   lua_pushboolean(L, true);

   return 1;
}

/**
 * lutro.window.maximize
 *
 * https://love2d.org/wiki/love.window.maximize
 */
int win_maximize(lua_State *L)
{
   int n = lua_gettop(L);

   if (n > 0)
      return luaL_error(L, "lutro.window.maximize expects 0 arguments, %d given.", n);

   // Lutro does not support maximizing the window.
   return 0;
}

/**
 * lutro.window.minimize
 *
 * https://love2d.org/wiki/love.window.minimize
 */
int win_minimize(lua_State *L)
{
   int n = lua_gettop(L);

   if (n > 0)
      return luaL_error(L, "lutro.window.minimize expects 0 arguments, %d given.", n);

   // Lutro does not support minimizing the window.
   return 0;
}

/**
 * lutro.window.getTitle
 *
 * https://love2d.org/wiki/love.window.getTitle
 */
int win_getTitle(lua_State *L)
{
   int n = lua_gettop(L);

   if (n > 0)
      return luaL_error(L, "lutro.window.getTitle expects 0 arguments, %d given.", n);

   // Return a standard title.
   lua_pushstring(L, "Lutro");

   return 1;
}

/**
 * lutro.window.setPosition
 *
 * https://love2d.org/wiki/love.window.setPosition
 */
int win_setPosition(lua_State *L)
{
   int n = lua_gettop(L);
   if (n < 2 || n > 3)
      return luaL_error(L, "lutro.window.setPosition expects 2-3 arguments, %d given.", n);

   // Setting position in Lutro is ignored.
   return 0;
}

/**
 * lutro.window.getPosition
 *
 * https://love2d.org/wiki/love.window.getPosition
 */
int win_getPosition(lua_State *L)
{
   int n = lua_gettop(L);
   if (n > 0)
      return luaL_error(L, "lutro.window.getPosition expects 0 arguments, %d given.", n);

   // The position is a fixed point.
   lua_pushnumber(L, 0); // x
   lua_pushnumber(L, 0); // y
   lua_pushnumber(L, 1); // display

   return 3;
}

/**
 * lutro.window.requestAttention
 *
 * https://love2d.org/wiki/love.window.requestAttention
 */
int win_requestAttention(lua_State *L)
{
   int n = lua_gettop(L);
   if (n < 0 || n > 1)
      return luaL_error(L, "lutro.window.requestAttention expects 0 or 1 arguments, %d given.", n);

   return 0;
}

/**
 * lutro.window.getDisplayName
 *
 * https://love2d.org/wiki/love.window.getDisplayName
 */
int win_getDisplayName(lua_State *L)
{
   int n = lua_gettop(L);
   if (n < 0 || n > 1)
      return luaL_error(L, "lutro.window.getDisplayName expects 0 or 1 arguments, %d given.", n);

   // Lutro does not support multiple displays.
   lua_pushstring(L, "libretro");

   return 1;
}

/**
 * lutro.window.setDisplaySleepEnabled
 *
 * https://love2d.org/wiki/love.window.setDisplaySleepEnabled
 */
int win_setDisplaySleepEnabled(lua_State *L)
{
   int n = lua_gettop(L);
   if (n != 1)
      return luaL_error(L, "lutro.window.win_setDisplaySleepEnabled expects 1 argument, %d given.", n);

   // Ignore setting the display sleep.
   return 0;
}

/**
 * lutro.window.isDisplaySleepEnabled
 *
 * https://love2d.org/wiki/love.window.isDisplaySleepEnabled
 */
int win_isDisplaySleepEnabled(lua_State *L)
{
   int n = lua_gettop(L);
   if (n != 0)
      return luaL_error(L, "lutro.window.isDisplaySleepEnabled expects 1 arguments, %d given.", n);

   // Lutro does not support sleeping displays.
   lua_pushboolean(L, 0);

   return 1;
}

/**
 * lutro.window.showMessageBox
 *
 * https://love2d.org/wiki/love.window.showMessageBox
 */
int win_showMessageBox(lua_State *L)
{
   int n = lua_gettop(L);
   if (n < 2)
      return luaL_error(L, "lutro.window.win_showMessageBox expects at least 2 arguments, %d given.", n);
   if (n > 5)
      return luaL_error(L, "lutro.window.win_showMessageBox expects at most 5 arguments, %d given.", n);

   // TODO: lutro.window.showMessageBox() - Prepend the title to the message... "title: message"
   // const char* title = luaL_checkstring(L, 1);
   const char* message = luaL_checkstring(L, 2);
   struct retro_message msg = { message, 600 };

   (*settings.environ_cb)(RETRO_ENVIRONMENT_SET_MESSAGE, &msg);

   lua_pushboolean(L, 1); // success

   return 1;
}
