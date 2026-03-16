#ifndef LUTRO_MATH_H
#define LUTRO_MATH_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "runtime.h"

void lutro_math_init(void);
int lutro_math_preload(lua_State *L);
int lutro_math_random(lua_State *L);
int lutro_math_setRandomSeed(lua_State *L);

#endif // LUTRO_MATH_H
