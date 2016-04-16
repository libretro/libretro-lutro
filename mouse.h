#ifndef MOUSE_H
#define MOUSE_H

#include <stdlib.h>
#include <string.h>
#include "runtime.h"

int lutro_mouse_preload(lua_State *L);
void lutro_mouse_init();
void lutro_mouseevent(lua_State* L);
int mouse_isDown(lua_State *L);

#endif // MOUSE_H