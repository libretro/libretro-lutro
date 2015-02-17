#ifndef RUNTIME_H
#define RUNTIME_H

#include "lua/src/lua.h"
#include "lua/src/lauxlib.h"
#include "lua/src/lualib.h"

int lutro_preload(lua_State *L, lua_CFunction f, const char *name);
int lutro_ensure_global_table(lua_State *L, const char *name);
void lutro_namespace(lua_State *L);

void lutro_stack_dump(lua_State* L);

#endif // RUNTIME_H
