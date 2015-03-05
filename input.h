#ifndef INPUT_H
#define INPUT_H

#include "runtime.h"

extern const struct int_const_map {
   long value;
   const char *name;
} joystick_enum[17];

int lutro_input_preload(lua_State *L);
int input_joypad(lua_State *L);
const char* input_find_name(const struct int_const_map*, unsigned);

#endif // INPUT_H
