#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "runtime.h"

void lutro_timer_init(void);
int lutro_timer_preload(lua_State *L);

int timer_getTime(lua_State *L);
int timer_getDelta(lua_State *L);
int timer_getFPS(lua_State *L);

#endif // TIMER_H
