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

void lutro_joystick_init(retro_environment_t cb)
{
}

void lutro_joystickevent(lua_State* L)
{
   int i, u;
   for (i = 0; i < 4; i++) {
   	for (u = 0; u < 14; u++) {
      	joystick_cache[i][u] = settings.input_cb(i, RETRO_DEVICE_JOYPAD, 0, u);
   	}
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
}

/**
 * Retrieve the libretro enum key from a Joystick string.
 */
int joystick_joystickToRetro(const char* retroKey)
{
  switch (retroKey[0]) {
    case "b":
      return RETRO_DEVICE_ID_JOYPAD_B;
    case "y":
      return RETRO_DEVICE_ID_JOYPAD_Y;
    case "s":
      switch (retroKey[1]) {
        case "e":
          return RETRO_DEVICE_ID_JOYPAD_SELECT;
        case "t":
          return RETRO_DEVICE_ID_JOYPAD_START;
      }
      break;
    case "u":
      return RETRO_DEVICE_ID_JOYPAD_UP;
    case "d":
      return RETRO_DEVICE_ID_JOYPAD_DOWN;
    case "l":
      switch (retroKey[1]) {
        case "e":
          return RETRO_DEVICE_ID_JOYPAD_LEFT;
        case "1":
          return RETRO_DEVICE_ID_JOYPAD_L; 
        case "2":
      return RETRO_DEVICE_ID_JOYPAD_L2;
        case "3":
          return RETRO_DEVICE_ID_JOYPAD_L3;
      }
      break;
    case "r":
      switch (retroKey[1]) {
        case "i":
          return RETRO_DEVICE_ID_JOYPAD_RIGHT;
        case "1":
          return RETRO_DEVICE_ID_JOYPAD_R;
        case "2":
          return RETRO_DEVICE_ID_JOYPAD_R2;
        case "3":
          return RETRO_DEVICE_ID_JOYPAD_R3;
      }
      break;
    case "a":
      return RETRO_DEVICE_ID_JOYPAD_A;
    case "x":
      return RETRO_DEVICE_ID_JOYPAD_X;
  }
}
