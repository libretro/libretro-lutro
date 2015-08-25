#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "runtime.h"

void lutro_system_init();
int lutro_system_preload(lua_State *L);

int sys_getOS(lua_State *L);

#endif // SYSTEM_H
