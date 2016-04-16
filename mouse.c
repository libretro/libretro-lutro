#include "mouse.h"
#include "lutro.h"
#include "libretro.h"

#include <stdlib.h>
#include <string.h>

static int16_t mouse_cache[8];

int lutro_mouse_preload(lua_State *L)
{
   static luaL_Reg mouse_funcs[] =  {
      { "isDown", mouse_isDown },
      {NULL, NULL}
   };

   lutro_ensure_global_table(L, "lutro");

   luaL_newlib(L, mouse_funcs);

   lua_setfield(L, -2, "mouse");

   return 1;
}

void lutro_mouse_init()
{
}

void lutro_mouseevent(lua_State* L)
{
   unsigned i;
   for (i = 0; i < 8; i++)
   {
      int16_t value = settings.input_cb(0, RETRO_DEVICE_MOUSE, 0, i);
      mouse_cache[i] = value;
   }
}

/**
 * lutro.mouse.isDown()
 *
 * https://love2d.org/wiki/love.mouse.isDown
 */
int mouse_isDown(lua_State *L)
{
    int n = lua_gettop(L);
    if (n < 1) {
        return luaL_error(L, "lutro.mouse.isDown requires 1 or more arguments, %d given.", n);
    }

    int buttonToCheck = 0;
    bool output = false;
    unsigned i;
    for (i = 0; i < n; i++) {
        buttonToCheck = (int) luaL_checknumber(L, i + 1);
        if (buttonToCheck == 1) {
            buttonToCheck = RETRO_DEVICE_ID_MOUSE_LEFT;
        }
        else if (buttonToCheck == 2) {
            buttonToCheck = RETRO_DEVICE_ID_MOUSE_RIGHT;
        }
        else if (buttonToCheck == 3) {
            buttonToCheck = RETRO_DEVICE_ID_MOUSE_MIDDLE;
        }
        else {
            buttonToCheck = 0;
        }
        if (buttonToCheck > 0) {
            if (mouse_cache[buttonToCheck]) {
                output = true;
                break;
            }
        }
    }
    lua_pushboolean(L, output);

    return 1;
}
