#ifndef WINDOW_H
#define WINDOW_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "runtime.h"

void lutro_window_init();
int lutro_window_preload(lua_State *L);

int win_setTitle(lua_State *L);
int win_setMode(lua_State *L);

#endif // WINDOW_H
