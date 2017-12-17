#ifndef EVENT_H
#define EVENT_H

#include <stdlib.h>
#include <string.h>

#include "runtime.h"

int lutro_event_preload(lua_State *L);
void lutro_event_init(void);
int event_quit(lua_State *L);

#endif // EVENT_H
