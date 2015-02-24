#ifndef LIVE_H
#define LIVE_H

#include "runtime.h"

int lutro_live_preload(lua_State *L);
void lutro_live_init();
void lutro_live_deinit();
void lutro_live_update(lua_State *L);
void lutro_live_draw();

#endif // LIVE_H
