#include "keyboard.h"
#include "lutro.h"
#include "libretro.h"

#include <stdlib.h>
#include <string.h>

static int16_t keyboard_cache[255];

int lutro_keyboard_preload(lua_State *L)
{
   static luaL_Reg keyboard_funcs[] =  {
      { "isDown", keyboard_isDown },
      { "getScancodeFromKey", keyboard_getScancodeFromKey },
      {NULL, NULL}
   };

   lutro_ensure_global_table(L, "lutro");

   luaL_newlib(L, keyboard_funcs);

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
        return luaL_error(L, "lutro.keyboard.isDown requires 1 or more arguments, %d given.", n);
    }

    int buttonToCheckKey;
    bool output = false;
    unsigned i;
    for (i = 0; i < n; i++) {
      const char* buttonToCheck = luaL_checkstring(L, i + 1);
      buttonToCheckKey = keyboard_string_to_libretro(buttonToCheck);
      if (keyboard_cache[buttonToCheckKey]) {
          output = true;
          break;
      }
    }
    lua_pushboolean(L, output);

    return 1;
}

/**
 * lutro.keyboard.getScancodeFromKey(key)
 *
 * https://love2d.org/wiki/love.keyboard.getScancodeFromKey
 */
int keyboard_getScancodeFromKey(lua_State *L) {
  int n = lua_gettop(L);
  if (n < 1) {
      return luaL_error(L, "lutro.keyboard.getScancodeFromKey requires 1 or more arguments, %d given.", n);
  }

  const char* buttonToCheck = luaL_checkstring(L, 1);
  int scancode = keyboard_string_to_libretro(buttonToCheck);
  lua_pushnumber(L, scancode);

  return 1;
}

/**
 * Converts a Love2D KeyCode string constant to a libretro constant.
 */
int keyboard_string_to_libretro(const char* key)
{
  switch (key[0]) {
    case 'a':
      return RETROK_a;
    case 'b':
      return RETROK_b;
    case 'c':
      return RETROK_c;
    case 'd':
      switch (key[1]) {
        case 'o':
          return RETROK_DOWN;
      }
      return RETROK_d;
    case 'e':
      return RETROK_e;
    case 'f':
      return RETROK_f;
    case 'g':
      return RETROK_g;
    case 'h':
      return RETROK_h;
    case 'i':
      return RETROK_i;
    case 'j':
      return RETROK_j;
    case 'k':
      return RETROK_k;
    case 'l':
      switch (key[1]) {
        case 'e':
          return RETROK_LEFT;
      }
      return RETROK_l;
    case 'm':
      return RETROK_m;
    case 'n':
      return RETROK_n;
    case 'o':
      return RETROK_o;
    case 'p':
      return RETROK_p;
    case 'q':
      return RETROK_q;
    case 'r':
      switch (key[1]) {
        case 'i':
          return RETROK_RIGHT;
      }
      return RETROK_r;
    case 's':
      switch (key[1]) {
        case 'p':
          return RETROK_SPACE;
      }
      return RETROK_s;
    case 't':
      return RETROK_t;
    case 'u':
      switch (key[1]) {
        case 'p':
          return RETROK_UP;
      }
      return RETROK_u;
    case 'v':
      return RETROK_v;
    case 'w':
      return RETROK_w;
    case 'x':
      return RETROK_x;
    case 'y':
      return RETROK_y;
    case 'z':
      return RETROK_z;
    case '0':
      return RETROK_0;
    case '1':
      return RETROK_1;
    case '2':
      return RETROK_2;
    case '3':
      return RETROK_3;
    case '4':
      return RETROK_4;
    case '5':
      return RETROK_5;
    case '6':
      return RETROK_6;
    case '7':
      return RETROK_7;
    case '8':
      return RETROK_8;
    case '9':
      return RETROK_9;
    case '!':
      return RETROK_EXCLAIM;
    case '"':
      return RETROK_QUOTEDBL;
    case '#':
      return RETROK_HASH;
    case '$':
      return RETROK_DOLLAR;
    case '&':
      return RETROK_AMPERSAND;
    case '\'':
      return RETROK_BACKSLASH;
    case '(':
      return RETROK_LEFTBRACKET;
    case ')':
      return RETROK_RIGHTBRACKET;
    case '*':
      return RETROK_KP_MULTIPLY;
    case '+':
      return RETROK_PLUS;
    case ',':
      return RETROK_COMMA;
    case '-':
      return RETROK_MINUS;
    case '/':
      return RETROK_SLASH;
    case ':':
      return RETROK_COLON;
    case ';':
      return RETROK_SEMICOLON;
    case '<':
      return RETROK_LESS;
    case '=':
      return RETROK_EQUALS;
    case '>':
      return RETROK_GREATER;
    case '?':
      return RETROK_QUESTION;
    case '@':
      return RETROK_ASTERISK;
    case '[':
      return RETROK_LEFTBRACKET;
    case '\\':
      return RETROK_BACKSLASH;
    case ']':
      return RETROK_RIGHTBRACKET;
    case '^':
      return RETROK_CARET;
    case '_':
      return RETROK_UNDERSCORE;
    case '`':
      return RETROK_CARET;
/*
    case 'home':
      return ;
    case 'end':
      return ;
    case 'pageup':
      return ;
    case 'pagedown':
      return ;
    case 'insert':
      return ;
    case 'backspace':
      return ;
    case 'tab':
      return ;
    case 'clear':
      return ;
    case 'return':
      return ;
    case 'delete':
      return ;
    case 'f1':
      return ;
    case 'f2':
      return ;
    case 'f3':
      return ;
    case 'f4':
      return ;
    case 'f5':
      return ;
    case 'f6':
      return ;
    case 'f7':
      return ;
    case 'f8':
      return ;
    case 'f9':
      return ;
    case 'f10':
      return ;
    case 'f11':
      return ;
    case 'f12':
      return ;
    case 'f13':
      return ;
    case 'f14':
      return ;
    case 'f15':
      return ;
    case 'f16':
      return ;
    case 'f17':
      return ;
    case 'f18':
      return ;
    case 'numlock':
      return ;
    case 'capslock':
      return ;
    case 'scrolllock':
      return ;
    case 'rshift':
      return ;
    case 'lshift':
      return ;
    case 'rctrl':
      return ;
    case 'lctrl':
      return ;
    case 'ralt':
      return ;
    case 'lalt':
      return ;
    case 'rgui':
      return ;
    case 'lgui':
      return ;
    case 'mode':
      return ;
    case 'www':
      return ;
    case 'mail':
      return ;
    case 'calculator':
      return ;
    case 'computer':
      return ;
    case 'appsearch':
      return ;
    case 'apphome':
      return ;
    case 'appback':
      return ;
    case 'appforward':
      return ;
    case 'apprefresh':
      return ;
    case 'appbookmarks':
      return ;
    case 'pause':
      return ;
    case 'escape':
      return ;
    case 'help':
      return ;
    case 'printscreen':
      return ;
    case 'sysreq':
      return ;
    case 'menu':
      return ;
    case 'application':
      return ;
    case 'power':
      return ;
    case 'currencyunit':
      return ;
    case 'undo':
      return ;
*/
  }

  return RETROK_UNKNOWN;
}
