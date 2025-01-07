#include <stdlib.h>
#include <string.h>

#include <libretro.h>

#include "event.h"
#include "lutro.h"

int lutro_event_preload(lua_State *L)
{
   static const luaL_Reg event_funcs[] =  {
      { "quit", event_quit },
      {NULL, NULL}
   };

   lutro_newlib(L, event_funcs, "event");

   return 1;
}

void lutro_event_init()
{
}

/**
 * lutro.event.quit()
 *
 * https://love2d.org/wiki/love.event.quit
 */
int event_quit(lua_State *L)
{
   int n = lua_gettop(L);
   if (n > 1) {
      return luaL_error(L, "lutro.event.quit requires 0 or 1 arguments, %d given.", n);
   }

   // Quit, ignoring the exit status.
   lutro_shutdown_game();

   return 0;
}
