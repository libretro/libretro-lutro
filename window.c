#include "window.h"
#include "lutro.h"
#include "graphics.h"
#include "libretro.h"

#include <stdlib.h>
#include <string.h>

int lutro_window_preload(lua_State *L)
{
   static luaL_Reg win_funcs[] =  {
      { "close", win_close },
      { "setTitle", win_setTitle },
      { "setMode",  win_setMode },
      { "setIcon", win_setIcon },
      { "isCreated", win_isCreated },
      {NULL, NULL}
   };

   lutro_ensure_global_table(L, "lutro");

   luaL_newlib(L, win_funcs);

   lua_setfield(L, -2, "window");

   return 1;
}

void lutro_window_init()
{
}

int win_setTitle(lua_State *L)
{
   return 0;
}

/**
 * lutro.window.close()
 *
 * https://love2d.org/wiki/love.window.close
 */
int win_close(lua_State *L)
{
   retro_shutdown_game();

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
