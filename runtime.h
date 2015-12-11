#ifndef RUNTIME_H
#define RUNTIME_H

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

/* functions to help check the stack size in a function call or block of code */
#ifndef NDEBUG
#define lutro_checked_stack_begin() int __stack = lua_gettop(L)
#define lutro_checked_stack_return(delta) \
   do {\
      lutro_checked_stack_assert(delta);\
      return delta;\
   } while(0)
#define lutro_checked_stack_assert(delta) \
   do {\
      int __stack_delta = (lua_gettop(L)-__stack);\
      if (__stack_delta != delta) {\
         fprintf(stderr, "Stack delta assertion failed: delta=%i expected=%i.\n", delta, __stack_delta);\
         lutro_stack_dump(L);\
         fflush(stdout);\
         fflush(stderr);\
         abort();\
      }\
   } while(0)
#else
#define lutro_checked_stack_begin()
#define lutro_checked_stack_return(delta)
#define lutro_checked_stack_end(delta)
#endif

int lutro_preload(lua_State *L, lua_CFunction f, const char *name);
int lutro_ensure_global_table(lua_State *L, const char *name);
void lutro_namespace(lua_State *L);
void lutro_stack_dump(lua_State *L);

/**
 * @brief Import a lua module
 *
 * If an error occurs, the error string will be pushed to the stack.
 *
 * @param L
 * @param modname    module name
 * @param pop_result whether to pop the require() result
 * @return whether the operation succeeded
 */
int lutro_require(lua_State *L, const char *modname, int pop_result);

/**
 * @brief Converts a relative path to a module name
 *
 * *outmod* must hold at least strlen(relpath)+1
 *
 * @param outmod  output module name
 * @param relpath relative path to the module
 */
void lutro_relpath_to_modname(char *outmod, const char *relpath);

/**
 * @brief Returns a "not implemented" error
 *
 * @param L
 * @return
 */
int l_not_implemented(lua_State *L);

#if LUA_VERSION_NUM < 502
#define LUA_OPEQ	0
#define LUA_OPLT	1
#define LUA_OPLE	2

#define luaL_newlibtable(L,l)  \
  lua_createtable(L, 0, sizeof(l)/sizeof((l)[0]) - 1)

#define luaL_newlib(L,l)       (luaL_newlibtable(L,l), luaL_setfuncs(L,l,0))

#define lua_pushglobaltable(L)  \
   lua_pushvalue(L, LUA_GLOBALSINDEX)

LUALIB_API void luaL_setfuncs (lua_State *L, const luaL_Reg *l, int nup);
LUA_API int lua_absindex (lua_State *L, int idx);
LUA_API int lua_compare (lua_State *L, int index1, int index2, int op);
LUALIB_API unsigned luaL_checkunsigned (lua_State *L, int narg);
#endif
#endif // RUNTIME_H
