#include "system.h"
#include "lutro.h"

#include <stdlib.h>
#include <string.h>

int lutro_system_preload(lua_State *L)
{
   static luaL_Reg sys_funcs[] =  {
      { "getOS", sys_getOS },
      {NULL, NULL}
   };

   lutro_ensure_global_table(L, "lutro");

   luaL_newlib(L, sys_funcs);

   lua_setfield(L, -2, "system");

   return 1;
}

void lutro_system_init()
{
}

int sys_getOS(lua_State *L)
{
   lua_pushstring(L, "Lutro");

   return 1;
}
