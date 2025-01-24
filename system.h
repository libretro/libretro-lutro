#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "runtime.h"

void lutro_system_init(void);
int lutro_system_preload(lua_State *L);

int sys_getOS(lua_State *L);
int sys_getProcessorCount(lua_State *L);
int sys_getClipboardText(lua_State *L);
int sys_setClipboardText(lua_State *L);
int sys_getPowerInfo(lua_State *L);
int sys_openURL(lua_State *L);
int sys_vibrate(lua_State *L);

#endif // SYSTEM_H
