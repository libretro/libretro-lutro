#include "timer.h"
#include "lutro.h"

#include <stdlib.h>
#include <string.h>

int lutro_timer_preload(lua_State *L)
{
   static luaL_Reg gfx_funcs[] =  {
      { "getTime", timer_getTime },
      {NULL, NULL}
   };

   lutro_ensure_global_table(L, "lutro");

   luaL_newlib(L, gfx_funcs);

   lua_setfield(L, -2, "timer");

   return 1;
}

void lutro_timer_init()
{
}

int timer_getTime(lua_State *L)
{
   lua_pushnumber(L, perf_cb.get_time_usec() / 1000000.0);

   return 1;
}
