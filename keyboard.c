#include "mouse.h"
#include "lutro.h"
#include "libretro.h"

#include <stdlib.h>
#include <string.h>

static int16_t keyboard_cache[255];

int lutro_keyboard_preload(lua_State *L)
{
   static luaL_Reg mouse_funcs[] =  {
      { "isDown", mouse_isDown },
      {NULL, NULL}
   };

   lutro_ensure_global_table(L, "lutro");

   luaL_newlib(L, mouse_funcs);

   lua_setfield(L, -2, "keyboard");

   return 1;
}

void lutro_keyboard_init()
{
}

void lutro_keyboardevent(lua_State* L)
{
   unsigned i;
   for (i = 0; i < 255; i++)
   {
      keyboard_cache[i] = settings.input_cb(0, RETRO_DEVICE_KEYBOARD, 0, i);
   }
}

/**
 * lutro.keyboard.isDown()
 *
 * https://love2d.org/wiki/love.keyboard.isDown
 */
int keyboard_isDown(lua_State *L)
{
    int n = lua_gettop(L);
    if (n < 1) {
        return luaL_error(L, "lutro.mouse.keyboard requires 1 or more arguments, %d given.", n);
    }

    char* buttonToCheck;
    int buttonToCheckKey;
    bool output = false;
    unsigned i;
    for (i = 0; i < n; i++) {
      buttonToCheck = luaL_checkstring(L, i + 1);
      buttonToCheckKey = keyboard_constant_to_retro(buttonToCheck);
      if (mouse_cache[buttonToCheckKey]) {
          output = true;
          break;
      }
    }
    lua_pushboolean(L, output);

    return 1;
}

int keyboard_constant_to_retro(char* key)
{
  switch (key[0]) {
    case 'a':
      return RETROK_a;
    break;
  }
}