#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdlib.h>
#include <string.h>
#include "runtime.h"

int lutro_keyboard_preload(lua_State *L);
void lutro_keyboard_init();
void lutro_keyboardevent(lua_State* L);
int keyboard_isDown(lua_State *L);
int keyboard_string_to_libretro(const char* key);

#endif // KEYBOARD_H
