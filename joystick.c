#include "joystick.h"
#include "lutro.h"
#include "libretro.h"

#include <stdlib.h>
#include <string.h>

static int16_t joystick_cache[4][14];

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
 * Invoke lutro.joystickpressed(joystick, button)
 * Invoke lutro.joystickreleased(joystick, button)
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
char* joystick_retroToJoystick(int joystickKey)
{
  switch (joystickKey) {
    case RETRO_DEVICE_ID_JOYPAD_B:
      return "b";
    case RETRO_DEVICE_ID_JOYPAD_Y:
      return "y";
    case RETRO_DEVICE_ID_JOYPAD_SELECT:
      return "select";
    case RETRO_DEVICE_ID_JOYPAD_START:
      return "start";
    case RETRO_DEVICE_ID_JOYPAD_UP:
      return "up";
    case RETRO_DEVICE_ID_JOYPAD_DOWN:
      return "down";
    case RETRO_DEVICE_ID_JOYPAD_LEFT:
      return "left";
    case RETRO_DEVICE_ID_JOYPAD_RIGHT:
      return "right";
    case RETRO_DEVICE_ID_JOYPAD_A:
      return "a";
    case RETRO_DEVICE_ID_JOYPAD_X:
      return "x";
    case RETRO_DEVICE_ID_JOYPAD_L: 
      return "l1";
    case RETRO_DEVICE_ID_JOYPAD_R:
      return "r1";
    case RETRO_DEVICE_ID_JOYPAD_L2:
      return "l2";
    case RETRO_DEVICE_ID_JOYPAD_R2:
      return "r2";
    case RETRO_DEVICE_ID_JOYPAD_L3:
      return "l3";
    case RETRO_DEVICE_ID_JOYPAD_R3:
      return "r3";
  }

  return "";
}

/**
 * Retrieve the libretro enum key from a Joystick string.
 *
 * @return The joystick key representing the string. -1 otherwise.
 */
int joystick_joystickToRetro(const char* retroKey)
{
  switch (retroKey[0]) {
    case 'b':
      return RETRO_DEVICE_ID_JOYPAD_B;
    case 'y':
      return RETRO_DEVICE_ID_JOYPAD_Y;
    case 's':
      switch (retroKey[1]) {
        case 'e':
          return RETRO_DEVICE_ID_JOYPAD_SELECT;
        case 't':
          return RETRO_DEVICE_ID_JOYPAD_START;
      }
      break;
    case 'u':
      return RETRO_DEVICE_ID_JOYPAD_UP;
    case 'd':
      return RETRO_DEVICE_ID_JOYPAD_DOWN;
    case 'l':
      switch (retroKey[1]) {
        case 'e':
          return RETRO_DEVICE_ID_JOYPAD_LEFT;
        case '1':
          return RETRO_DEVICE_ID_JOYPAD_L; 
        case '2':
      return RETRO_DEVICE_ID_JOYPAD_L2;
        case '3':
          return RETRO_DEVICE_ID_JOYPAD_L3;
      }
      break;
    case 'r':
      switch (retroKey[1]) {
        case 'i':
          return RETRO_DEVICE_ID_JOYPAD_RIGHT;
        case '1':
          return RETRO_DEVICE_ID_JOYPAD_R;
        case '2':
          return RETRO_DEVICE_ID_JOYPAD_R2;
        case '3':
          return RETRO_DEVICE_ID_JOYPAD_R3;
      }
      break;
    case 'a':
      return RETRO_DEVICE_ID_JOYPAD_A;
    case 'x':
      return RETRO_DEVICE_ID_JOYPAD_X;
  }

  return -1;
}
