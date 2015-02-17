#include "input.h"
#include "lutro.h"

int lutro_input_preload(lua_State *L)
{
   static luaL_Reg funcs[] =  {
      {"joypad", input_joypad},
      {NULL, NULL}
   };

   lutro_ensure_global_table(L, "lutro");

   luaL_newlib(L, funcs);

   lua_pushnumber(L, RETRO_DEVICE_ID_JOYPAD_UP);
   lua_setfield(L, -2, "JOY_UP");
   lua_pushnumber(L, RETRO_DEVICE_ID_JOYPAD_DOWN);
   lua_setfield(L, -2, "JOY_DOWN");
   lua_pushnumber(L, RETRO_DEVICE_ID_JOYPAD_LEFT);
   lua_setfield(L, -2, "JOY_LEFT");
   lua_pushnumber(L, RETRO_DEVICE_ID_JOYPAD_RIGHT);
   lua_setfield(L, -2, "JOY_RIGHT");
   lua_pushnumber(L, RETRO_DEVICE_ID_JOYPAD_A);
   lua_setfield(L, -2, "JOY_A");
   lua_pushnumber(L, RETRO_DEVICE_ID_JOYPAD_B);
   lua_setfield(L, -2, "JOY_B");
   lua_pushnumber(L, RETRO_DEVICE_ID_JOYPAD_START);
   lua_setfield(L, -2, "JOY_START");

   lua_setfield(L, -2, "input");

   return 1;
}

int input_joypad(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 1)
      return luaL_error(L, "lutro.input.joypad requires at least one argument, %i given.", n);

   unsigned id = luaL_checkunsigned(L, 1);
   unsigned port = 0;
   unsigned index = 0;

   if (n > 1)
      port = luaL_checkunsigned(L, 2);

   if (n > 2)
      index = luaL_checkunsigned(L, 3);

   lua_pop(L, n);

   int value = settings.input_cb(port, RETRO_DEVICE_JOYPAD, index, id);

   lua_pushinteger(L, value);

   return 1;
}
