#include "timer.h"
#include "lutro.h"

#include <stdlib.h>
#include <string.h>

int lutro_timer_preload(lua_State *L)
{
   static const luaL_Reg timer_funcs[] =  {
      { "getTime", timer_getTime },
      { "getDelta", timer_getDelta },
      { "getFPS", timer_getFPS },
      {NULL, NULL}
   };

   lutro_newlib(L, timer_funcs, "timer");

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

/**
 * lutro.timer.getDelta()
 *
 * @see https://love2d.org/wiki/love.timer.getDelta
 */
int timer_getDelta(lua_State *L)
{
   lua_pushnumber(L, settings.delta);

   return 1;
}

/**
 * lutro.timer.getFPS()
 *
 * @see https://love2d.org/wiki/love.timer.getFPS
 */
int timer_getFPS(lua_State *L)
{
   lua_pushnumber(L, settings.fps);

   return 1;
}
