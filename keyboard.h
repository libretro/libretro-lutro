#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdlib.h>
#include <string.h>
#include "runtime.h"
#include "libretro.h"

extern const struct key_int_const_map {
   long value;
   const char *name;
} keyboard_enum[RETROK_LAST];

int lutro_keyboard_preload(lua_State *L);
void lutro_keyboard_init();
void lutro_keyboardevent(lua_State* L);
int keyboard_isDown(lua_State *L);
int keyboard_getKeyFromScancode(lua_State *L);
int keyboard_getScancodeFromKey(lua_State *L);
int keyboard_string_to_libretro(const char* key);
int keyboard_find_value(const struct key_int_const_map *map, const char *name, unsigned *value);
const char* keyboard_find_name(const struct key_int_const_map *map, unsigned value);

#endif // KEYBOARD_H
