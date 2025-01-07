#include "system.h"
#include "lutro.h"

#include <stdlib.h>
#include <string.h>

/**
 * Mocked clipboard text for internal Lutro usage.
 */
const char* clipboardText = "";

int lutro_system_preload(lua_State *L)
{
   static const luaL_Reg sys_funcs[] =  {
      { "getOS", sys_getOS },
      { "getProcessorCount", sys_getProcessorCount },
      { "setClipboardText", sys_setClipboardText },
      { "getClipboardText", sys_getClipboardText },
      { "getPowerInfo", sys_getPowerInfo },
      { "openURL", sys_openURL },
      { "vibrate", sys_vibrate },
      {NULL, NULL}
   };

   lutro_newlib(L, sys_funcs, "system");

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

int sys_getPowerInfo(lua_State *L)
{
   lua_pushstring(L, "unknown");

   return 1;
}

int sys_openURL(lua_State *L)
{
   lua_pushboolean(L, false);

   return 1;
}

int sys_vibrate(lua_State *L)
{
   return 0;
}

/**
 * lutro.system.getProcessorCount
 *
 * @see https://love2d.org/wiki/love.system.getProcessorCount
 */
int sys_getProcessorCount(lua_State *L)
{
   // Assume one thread is available, since Threading support is not there.
   lua_pushnumber(L, 1);

   return 1;
}

/**
 * lutro.system.setClipboardText
 *
 * @see https://love2d.org/wiki/love.system.setClipboardText
 */
int sys_setClipboardText(lua_State *L)
{
   int n = lua_gettop(L);
   if (n < 1) {
      return luaL_error(L, "lutro.system.setClipboardText requires 1 argument, %d given.", n);
   }

   // Lutro does not support setting the clipboard, but we can mock our own.
   clipboardText = luaL_checkstring(L, 1);

   return 0;
}

/**
 * lutro.system.getClipboardText
 *
 * @see https://love2d.org/wiki/love.system.getClipboardText
 */
int sys_getClipboardText(lua_State *L)
{
   int n = lua_gettop(L);
   if (n > 0) {
      return luaL_error(L, "lutro.system.getClipboardText requires 0 argument, %d given.", n);
   }

   // Lutro does not support retrieving from the clipboard, but we can use our
   // mock clipboard.
   lua_pushstring(L, clipboardText);

   return 1;
}
