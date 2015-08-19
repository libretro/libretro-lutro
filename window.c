#include "window.h"
#include "lutro.h"

#include <stdlib.h>
#include <string.h>

int lutro_window_preload(lua_State *L)
{
   static luaL_Reg win_funcs[] =  {
      { "setTitle", win_setTitle },
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
