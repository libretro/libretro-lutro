#include "keyboard.h"
#include "lutro.h"
#include "libretro.h"

#include <stdlib.h>
#include <string.h>

static int16_t keyboard_cache[RETROK_LAST];

const struct key_int_const_map keyboard_enum[RETROK_LAST] = {
   {RETROK_BACKSPACE     ,"backspace"},
   {RETROK_TAB           ,"tab"},
   {RETROK_CLEAR         ,"clear"},
   {RETROK_RETURN        ,"return"},
   {RETROK_PAUSE         ,"pause"},
   {RETROK_ESCAPE        ,"escape"},
   {RETROK_SPACE         ,"space"},
   {RETROK_EXCLAIM       ,"!"},
   {RETROK_QUOTEDBL      ,"\""},
   {RETROK_HASH          ,"#"},
   {RETROK_DOLLAR        ,"$"},
   {RETROK_AMPERSAND     ,"&"},
   {RETROK_QUOTE         ,"'"},
   {RETROK_LEFTPAREN     ,"("},
   {RETROK_RIGHTPAREN    ,")"},
   {RETROK_ASTERISK      ,"*"},
   {RETROK_PLUS          ,"+"},
   {RETROK_COMMA         ,","},
   {RETROK_MINUS         ,"-"},
   {RETROK_PERIOD        ,"."},
   {RETROK_SLASH         ,"/"},
   {RETROK_0             ,"0"},
   {RETROK_1             ,"1"},
   {RETROK_2             ,"2"},
   {RETROK_3             ,"3"},
   {RETROK_4             ,"4"},
   {RETROK_5             ,"5"},
   {RETROK_6             ,"6"},
   {RETROK_7             ,"7"},
   {RETROK_8             ,"8"},
   {RETROK_9             ,"9"},
   {RETROK_COLON         ,":"},
   {RETROK_SEMICOLON     ,";"},
   {RETROK_LESS          ,"<"},
   {RETROK_EQUALS        ,"="},
   {RETROK_GREATER       ,">"},
   {RETROK_QUESTION      ,"?"},
   {RETROK_AT            ,"@"},
   {RETROK_LEFTBRACKET   ,"["},
   {RETROK_BACKSLASH     ,"\\"},
   {RETROK_RIGHTBRACKET  ,"]"},
   {RETROK_CARET         ,"^"},
   {RETROK_UNDERSCORE    ,"_"},
   {RETROK_BACKQUOTE     ,"\""},
   {RETROK_a             ,"a"},
   {RETROK_b             ,"b"},
   {RETROK_c             ,"c"},
   {RETROK_d             ,"d"},
   {RETROK_e             ,"e"},
   {RETROK_f             ,"f"},
   {RETROK_g             ,"g"},
   {RETROK_h             ,"h"},
   {RETROK_i             ,"i"},
   {RETROK_j             ,"j"},
   {RETROK_k             ,"k"},
   {RETROK_l             ,"l"},
   {RETROK_m             ,"m"},
   {RETROK_n             ,"n"},
   {RETROK_o             ,"o"},
   {RETROK_p             ,"p"},
   {RETROK_q             ,"q"},
   {RETROK_r             ,"r"},
   {RETROK_s             ,"s"},
   {RETROK_t             ,"t"},
   {RETROK_u             ,"u"},
   {RETROK_v             ,"v"},
   {RETROK_w             ,"w"},
   {RETROK_x             ,"x"},
   {RETROK_y             ,"y"},
   {RETROK_z             ,"z"},
   {RETROK_DELETE        ,"kpdelete"},

   {RETROK_KP0           ,"kp0"},
   {RETROK_KP1           ,"kp1"},
   {RETROK_KP2           ,"kp2"},
   {RETROK_KP3           ,"kp3"},
   {RETROK_KP4           ,"kp4"},
   {RETROK_KP5           ,"kp5"},
   {RETROK_KP6           ,"kp6"},
   {RETROK_KP7           ,"kp7"},
   {RETROK_KP8           ,"kp8"},
   {RETROK_KP9           ,"kp9"},
   {RETROK_KP_PERIOD     ,"kp."},
   {RETROK_KP_DIVIDE     ,"kp/"},
   {RETROK_KP_MULTIPLY   ,"kp*"},
   {RETROK_KP_MINUS      ,"kp-"},
   {RETROK_KP_PLUS       ,"kp+"},
   {RETROK_KP_ENTER      ,"kpenter"},
   {RETROK_KP_EQUALS     ,"kp="},

   {RETROK_UP            ,"up"},
   {RETROK_DOWN          ,"down"},
   {RETROK_RIGHT         ,"right"},
   {RETROK_LEFT          ,"left"},
   {RETROK_INSERT        ,"insert"},
   {RETROK_HOME          ,"home"},
   {RETROK_END           ,"end"},
   {RETROK_PAGEUP        ,"pageup"},
   {RETROK_PAGEDOWN      ,"pagedown"},

   {RETROK_F1            ,"f1"},
   {RETROK_F2            ,"f2"},
   {RETROK_F3            ,"f3"},
   {RETROK_F4            ,"f4"},
   {RETROK_F5            ,"f5"},
   {RETROK_F6            ,"f6"},
   {RETROK_F7            ,"f7"},
   {RETROK_F8            ,"f8"},
   {RETROK_F9            ,"f9"},
   {RETROK_F10           ,"f10"},
   {RETROK_F11           ,"f11"},
   {RETROK_F12           ,"f12"},
   {RETROK_F13           ,"f13"},
   {RETROK_F14           ,"f14"},
   {RETROK_F15           ,"f15"},

   {RETROK_NUMLOCK       ,"numlock"},
   {RETROK_CAPSLOCK      ,"capslock"},
   {RETROK_SCROLLOCK     ,"scrolllock"},
   {RETROK_RSHIFT        ,"rshift"},
   {RETROK_LSHIFT        ,"lshift"},
   {RETROK_RCTRL         ,"rctrl"},
   {RETROK_LCTRL         ,"lctrl"},
   {RETROK_RALT          ,"ralt"},
   {RETROK_LALT          ,"lalt"},
   {RETROK_RMETA         ,"rmeta"},
   {RETROK_LMETA         ,"lmeta"},
   {RETROK_LSUPER        ,"lgui"},
   {RETROK_RSUPER        ,"rgui"},
   {RETROK_MODE          ,"mode"},
   {RETROK_COMPOSE       ,"application"},

   {RETROK_HELP          ,"help"},
   {RETROK_PRINT         ,"printscreen"},
   {RETROK_SYSREQ        ,"sysreq"},
   {RETROK_BREAK         ,"pause"},
   {RETROK_MENU          ,"menu"},
   {RETROK_POWER         ,"power"},
   {RETROK_EURO          ,"currencyunit"},
   {RETROK_UNDO          ,"undo"},
   {0, NULL}
};

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

/**
 * Keyboard Events
 *
 * https://love2d.org/wiki/love.keypressed
 * https://love2d.org/wiki/love.keyreleased
 */
void lutro_keyboardevent(lua_State* L)
{
   int16_t is_down;
   for (unsigned i = 0; i < RETROK_LAST; i++)
   {
      // Check if the keyboard key is pressed.
      is_down = settings.input_cb(0, RETRO_DEVICE_KEYBOARD, 0, i);
      // Change the state if needed.
      if (is_down != keyboard_cache[i]) {
         // Load up the lutro.keypressed event Lua function.
         lua_getfield(L, -1, is_down ? "keypressed" : "keyreleased");
         if (lua_isfunction(L, -1))
         {
            // Set up the arguments.
            lua_pushstring(L, keyboard_find_name(keyboard_enum, i)); // KeyConstant key
            lua_pushnumber(L, i); // Scancode scancode
            // @todo Add the isrepeat argument functionality. https://love2d.org/wiki/love.keypressed
            lua_pushboolean(L, false);

            // Call the function.
            if (lua_pcall(L, 3, 0, 0))
            {
               fprintf(stderr, "%s\n", lua_tostring(L, -1));
               lua_pop(L, 1);
            }
         }
         else
         {
            lua_pop(L, 1);
         }

         // Update the keyboard state.
         keyboard_cache[i] = is_down;
      }
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

    bool output = false;
    unsigned i;
    for (i = 0; i < n; i++) {
      const char* buttonToCheck = luaL_checkstring(L, i + 1);
      unsigned id;
      if (!keyboard_find_value(keyboard_enum, buttonToCheck, &id))
         return luaL_error(L, "invalid button");
      if (keyboard_cache[id]) {
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

  unsigned id;
  if (!keyboard_find_value(keyboard_enum, buttonToCheck, &id))
     return luaL_error(L, "invalid button");
  lua_pushnumber(L, id);

  return 1;
}

int keyboard_find_value(const struct key_int_const_map *map, const char *name, unsigned *value)
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

const char* keyboard_find_name(const struct key_int_const_map *map, unsigned value)
{
   for (; map->name; ++map)
      if (map->value == value)
         return map->name;
   return "";
}
