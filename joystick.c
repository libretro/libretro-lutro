#include "joystick.h"
#include "lutro.h"
#include "libretro.h"

#include <stdlib.h>
#include <string.h>

static int16_t joystick_cache[4][14];

const struct joystick_int_const_map joystick_key_enum[17] = {
   {RETRO_DEVICE_ID_JOYPAD_B, "b"},
   {RETRO_DEVICE_ID_JOYPAD_Y, "y"},
   {RETRO_DEVICE_ID_JOYPAD_SELECT, "select"},
   {RETRO_DEVICE_ID_JOYPAD_START, "start"},
   {RETRO_DEVICE_ID_JOYPAD_UP, "up"},
   {RETRO_DEVICE_ID_JOYPAD_DOWN, "down"},
   {RETRO_DEVICE_ID_JOYPAD_LEFT, "left"},
   {RETRO_DEVICE_ID_JOYPAD_RIGHT, "right"},
   {RETRO_DEVICE_ID_JOYPAD_A, "a"},
   {RETRO_DEVICE_ID_JOYPAD_X, "x"},
   {RETRO_DEVICE_ID_JOYPAD_L, "l1"},
   {RETRO_DEVICE_ID_JOYPAD_R, "r1"},
   {RETRO_DEVICE_ID_JOYPAD_L2, "l2"},
   {RETRO_DEVICE_ID_JOYPAD_R2, "r2"},
   {RETRO_DEVICE_ID_JOYPAD_L3, "l3"},
   {RETRO_DEVICE_ID_JOYPAD_R3, "r3"},
   {0, NULL}
};

int lutro_joystick_preload(lua_State *L)
{
   static luaL_Reg joystick_funcs[] =  {
      { "getJoystickCount", joystick_getJoystickCount },
      { "isDown", joystick_isDown },
      {NULL, NULL}
   };

   lutro_ensure_global_table(L, "lutro");

   luaL_newlib(L, joystick_funcs);

   lua_setfield(L, -2, "joystick");

   return 1;
}

void lutro_joystick_init()
{
}

void lutro_joystickevent(lua_State* L)
{
  int i, u;
  int16_t state;

  // Loop through each joystick.
  for (i = 0; i < 4; i++) {
    // Loop through each button.
    for (u = 0; u < 14; u++) {
      // Retrieve the state of the button.
      state = settings.input_cb(i, RETRO_DEVICE_JOYPAD, 0, u);

      // Check if there's a change of state.
      if (joystick_cache[i][u] != state) {
        joystick_cache[i][u] = state;
        // If the button was pressed, invoke the callback.
        if (state > 0) {
          lutro_joystickInvokeJoystickEvent(L, "joystickpressed", i, u);
        }
        else {
          lutro_joystickInvokeJoystickEvent(L, "joystickreleased", i, u);
        }
      }
    }
  }
}

/**
 * Invokes lutro.joystickpressed(joystick, button)
 * Invokes lutro.joystickreleased(joystick, button)
 */
void lutro_joystickInvokeJoystickEvent(lua_State* L, char* eventName, int joystick, int button) {
  lua_getglobal(L, "lutro");
  lua_getfield(L, -1, eventName);
  if (lua_isfunction(L, -1))
  {
    // Add the first argument (the joystick number).
    // TODO: Switch to using Joystick objects.
    lua_pushnumber(L, joystick);

    // Add the second argument (the joystick key).
    lua_pushnumber(L, button);

    // Call the event callback.
    if (lua_pcall(L, 2, 0, -5))
    {
       fprintf(stderr, "%s\n", lua_tostring(L, -1));
       lua_pop(L, 1);
    }
  } else {
    lua_pop(L, 1);
  }
}

/**
 * lutro.joystick.getJoystickCount()
 *
 * https://love2d.org/wiki/love.joystick.getJoystickCount
 */
int joystick_getJoystickCount(lua_State *L)
{
    int n = lua_gettop(L);
    if (n > 0) {
        return luaL_error(L, "lutro.joystick.getJoystickCount requires no arguments, %d given.", n);
    }

    // TODO: Query libretro to see device capacities of all joysticks.
    lua_pushnumber(L, 4);

    return 1;
}

/**
 * lutro.joystick.isDown() from LOVE 0.9.0.
 *
 * https://love2d.org/wiki/love.joystick.isDown
 */
int joystick_isDown(lua_State *L)
{
    int n = lua_gettop(L);
    if (n != 2) {
        return luaL_error(L, "lutro.joystick.isDown requires two arguments, %d given.", n);
    }

    bool output;

    int joystick = luaL_checknumber(L, 1);
    int button = luaL_checknumber(L, 2);

    output = (bool) joystick_cache[joystick - 1][button - 1];

    lua_pushboolean(L, output);

    return 1;
}

/**
 * Retrieve the Joystick string from a given libretro enum key.
 *
 * @return The string if the key is found, an empty string otherwise.
 */
const char* joystick_retroToJoystick(unsigned joystickKey)
{
  return joystick_find_name(joystick_key_enum, joystickKey);
}

/**
 * Retrieve the libretro enum key from a Joystick string.
 *
 * @return The joystick key representing the string. -1 otherwise.
 */
int joystick_joystickToRetro(const char* retroKey)
{
  unsigned id;
  if (joystick_find_value(joystick_key_enum, retroKey, &id) == 0) {
    return 0;
  }
  return id;
}

/**
 * Retrieve the value of the given joystick button.
 */
int joystick_find_value(const struct joystick_int_const_map *map, const char *name, unsigned *value)
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

/**
 * Finds the name of the given joystick key.
 */
const char* joystick_find_name(const struct joystick_int_const_map *map, unsigned value)
{
   for (; map->name; ++map)
      if (map->value == value)
         return map->name;
   return "";
}
