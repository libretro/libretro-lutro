#ifndef RUNTIME_H
#define RUNTIME_H

#include "lua/src/lua.h"
#include "lua/src/lauxlib.h"
#include "lua/src/lualib.h"

int lutro_preload(lua_State *L, lua_CFunction f, const char *name);
int lutro_ensure_global_table(lua_State *L, const char *name);
void lutro_namespace(lua_State *L);

void lutro_stack_dump(lua_State* L);

/* functions to help check the stack size in a function call or block of code */
#ifndef NDEBUG
#define lutro_checked_stack_begin() int __stack = lua_gettop(L)
#define lutro_checked_stack_return(delta) \
   do {\
      lutro_checked_stack_end(delta);\
      return delta;\
   } while(0)
#define lutro_checked_stack_end(delta) \
   do {\
      int __stack_delta = (lua_gettop(L)-__stack);\
      assert((__stack_delta == delta) && "Delta is not " #delta);\
   } while(0)
#else
#define lutro_checked_stack_begin()
#define lutro_checked_stack_return(delta)
#define lutro_checked_stack_end(delta)
#endif

#endif // RUNTIME_H
