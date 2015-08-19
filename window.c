#include "window.h"
#include "lutro.h"

#include <stdlib.h>
#include <string.h>

int lutro_window_preload(lua_State *L)
{
   static luaL_Reg win_funcs[] =  {
      { "setTitle", win_setTitle },
      { "setMode", win_setMode },
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

int win_setMode(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 3)
      return luaL_error(L, "lutro.window.setMode requires 3 arguments, %d given.", n);

   settings.width = luaL_checknumber(L, 1);
   settings.height = luaL_checknumber(L, 2);
   luaL_checktype(L, 3, LUA_TTABLE);

   lua_pop(L, n);

   lua_pushboolean(L, true);

   return 1;
}
