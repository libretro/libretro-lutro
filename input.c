#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "input.h"
#include "lutro.h"

const struct int_const_map joystick_enum[17] = {
   {RETRO_DEVICE_ID_JOYPAD_B,      "b"},
   {RETRO_DEVICE_ID_JOYPAD_Y,      "y"},
   {RETRO_DEVICE_ID_JOYPAD_SELECT, "select"},
   {RETRO_DEVICE_ID_JOYPAD_START,  "start"},
   {RETRO_DEVICE_ID_JOYPAD_UP,     "up"},
   {RETRO_DEVICE_ID_JOYPAD_DOWN,   "down"},
   {RETRO_DEVICE_ID_JOYPAD_LEFT,   "left"},
   {RETRO_DEVICE_ID_JOYPAD_RIGHT,  "right"},
   {RETRO_DEVICE_ID_JOYPAD_A,      "a"},
   {RETRO_DEVICE_ID_JOYPAD_X,      "x"},
   {RETRO_DEVICE_ID_JOYPAD_L,      "l1"},
   {RETRO_DEVICE_ID_JOYPAD_R,      "r1"},
   {RETRO_DEVICE_ID_JOYPAD_L2,     "l2"},
   {RETRO_DEVICE_ID_JOYPAD_R2,     "r2"},
   {RETRO_DEVICE_ID_JOYPAD_L3,     "l3"},
   {RETRO_DEVICE_ID_JOYPAD_R3,     "r3"},
   {0, NULL}
};

// TODO: ask somebody to add a hash table to libretro-common
int input_find_value(const struct int_const_map *map, const char *name, unsigned *value)
{
   for (; map->name; ++map)
   {
      if (strcmp(map->name, name) == 0)
      {
         *value = map->value;
         return 1;
      }
   }

   return 0;
}

const char* input_find_name(const struct int_const_map *map, unsigned value)
{
   for (; map->name; ++map)
      if (map->value == value)
         return map->name;
   return "";
}

int lutro_input_preload(lua_State *L)
{
   static const luaL_Reg funcs[] =  {
      {"joypad", input_joypad},
      {NULL, NULL}
   };

   lutro_newlib(L, funcs, "input");

   return 1;
}

int input_joypad(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 1)
      return luaL_error(L, "lutro.input.joypad requires at least one argument, %d given.", n);

   const char *idstr = luaL_checkstring(L, 1);
   unsigned id;

   if (!input_find_value(joystick_enum, idstr, &id))
      return luaL_error(L, "invalid button");

   unsigned port = 0;
   unsigned index = 0;

   // need to subtract 1 so the programmer can use lua-style indexing
   if (n > 1)
      port = luaL_checkunsigned(L, 2) - 1;

   if (n > 2)
      index = luaL_checkunsigned(L, 3) - 1;

   lua_pop(L, n);

   if (settings.input_cb(port, RETRO_DEVICE_JOYPAD, index, id))
   {
      lua_pushinteger(L, 1);
      return 1;
   }
   else
   {
      return 0;
   }
}
