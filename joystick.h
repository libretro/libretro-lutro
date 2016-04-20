#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <stdlib.h>
#include <string.h>
#include "runtime.h"
#include "libretro.h"

int lutro_joystick_preload(lua_State *L);
void lutro_joystick_init(retro_environment_t cb);
void lutro_joystickevent(lua_State* L);
int joystick_getJoystickCount(lua_State *L);

#endif // JOYSTICK_H
