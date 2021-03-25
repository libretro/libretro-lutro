#ifndef WINDOW_H
#define WINDOW_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "runtime.h"

void lutro_window_init();
int lutro_window_preload(lua_State *L);

int win_close(lua_State *L);
int win_setTitle(lua_State *L);
int win_setIcon(lua_State *L);
int win_setMode(lua_State *L);
int win_isCreated(lua_State *L);
int win_maximize(lua_State *L);
int win_minimize(lua_State *L);
int win_getTitle(lua_State *L);
int win_setPosition(lua_State *L);
int win_getPosition(lua_State *L);
int win_requestAttention(lua_State *L);
int win_getDisplayName(lua_State *L);
int win_setDisplaySleepEnabled(lua_State *L);
int win_isDisplaySleepEnabled(lua_State *L);
int win_showMessageBox(lua_State *L);

#endif // WINDOW_H
