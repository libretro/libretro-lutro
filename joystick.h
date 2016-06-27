#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <stdlib.h>
#include <string.h>
#include "runtime.h"
#include "libretro.h"

extern const struct joystick_int_const_map {
   long value;
   const char *name;
} joystick_key_enum[17];

int lutro_joystick_preload(lua_State *L);
void lutro_joystick_init();
void lutro_joystickevent(lua_State* L);
int joystick_getJoystickCount(lua_State *L);
int joystick_isDown(lua_State *L);
const char* joystick_retroToJoystick(unsigned joystickKey);
int joystick_joystickToRetro(const char* retroKey);
void lutro_joystickInvokeJoystickEvent(lua_State* L, char* eventName, int joystick, int button);
int joystick_find_value(const struct joystick_int_const_map *map, const char *name, unsigned *value);
const char* joystick_find_name(const struct joystick_int_const_map *map, unsigned value);

#endif // JOYSTICK_H
