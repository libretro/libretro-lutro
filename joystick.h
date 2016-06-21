#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <stdlib.h>
#include <string.h>
#include "runtime.h"
#include "libretro.h"

int lutro_joystick_preload(lua_State *L);
void lutro_joystick_init();
void lutro_joystickevent(lua_State* L);
int joystick_getJoystickCount(lua_State *L);
int joystick_isDown(lua_State *L);
char* joystick_retroToJoystick(int joystickKey);
int joystick_joystickToRetro(const char* retroKey);
void lutro_joystickInvokeJoystickEvent(lua_State* L, char* eventName, int joystick, int button);

#endif // JOYSTICK_H
