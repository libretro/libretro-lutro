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
