#ifndef RUNTIME_H
#define RUNTIME_H

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#if !defined(WANT_CHECKED_STACK)
#  if LUTRO_BUILD_IS_TOOL
#     define WANT_CHECKED_STACK   1
#  else
#     define WANT_CHECKED_STACK   0
#  endif
#endif

#  define _impl_checked_stack_begin_(L) int _stack__ = lua_gettop(L)
#  define _impl_checked_stack_end_(L, retvals) \
     ( ((lua_gettop(L) - _stack__) != retvals) && (lutro_checked_stack_assert(L, _stack__ + retvals, __FILE__, __LINE__), 1), retvals )

// performs stack checking in player/retail builds. Use only in situations where performance will not be impacted.
#define player_checked_stack_begin(L)        _impl_checked_stack_begin_(L)
#define player_checked_stack_end(L, retvals) _impl_checked_stack_end_(L, retvals)

#if WANT_CHECKED_STACK
// performs stack checking in tool/debug builds. Use liberally.
#  define tool_checked_stack_begin(L)        _impl_checked_stack_begin_(L)
#  define tool_checked_stack_end(L, retvals) _impl_checked_stack_end_(L, retvals)
#else
#  define tool_checked_stack_begin(L)         ((void)0) 
#  define tool_checked_stack_end(L, retvals)  (retvals)
#endif

void lutro_checked_stack_assert(lua_State* L, int expectedTop, char const* file, int line);


int lutro_preload(lua_State *L, lua_CFunction f, const char *name);
void lutro_namespace(lua_State *L);
void lutro_stack_dump(lua_State *L);
int lutro_getVersion(lua_State *L);

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

void luax_register(lua_State *L, const char *name, const luaL_Reg *l);
int luax_c_insistglobal(lua_State *L, const char *k);
void luax_setfuncs(lua_State *L, const luaL_Reg *l);

int traceback(lua_State *L);
int lutro_pcall(lua_State *L, int narg, int nret);
int lutro_pcall_isfunction(lua_State* L, int idx);

void lutro_newlib_x(lua_State* L, luaL_Reg const* funcs, char const* fieldname, int numfuncs);
#define lutro_newlib(L,funcs,fieldname)  (lutro_newlib_x(L,funcs,fieldname, (sizeof(funcs) / sizeof((funcs)[0]) - 1)))


#define luax_reqglobal(L, k)  ((void) (lua_getglobal(L, k), dbg_assertf(lua_istable(L, -1), "missing global table '%s'",k), 0) )

#endif // RUNTIME_H
