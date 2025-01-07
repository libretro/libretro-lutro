#include "runtime.h"
#include "compat/strl.h"
#include "file/file_path.h"
#include "lutro.h"
#include <assert.h>
#if LUA_VERSION_NUM < 502
#ifdef HAVE_JIT
#include "lj_obj.h"
#else
#include "lstate.h"
#endif
#endif

int lutro_preload(lua_State *L, lua_CFunction f, const char *name)
{
   lua_getglobal(L, "package");
   lua_getfield(L, -1, "preload");
   lua_pushcfunction(L, f);
   lua_setfield(L, -2, name);
   lua_pop(L, 2);

   return 0;
}

int lutro_ensure_global_table(lua_State *L, const char *name)
{
   lua_getglobal(L, name);

   if (!lua_istable(L, -1)) {
      lua_pop(L, 1);
      lua_newtable(L);

      // Introduce lutro.getVersion().
      lua_pushcfunction(L, lutro_getVersion);
      lua_setfield(L, -2, "getVersion");

      // Add the "lutro" Lua global.
      lua_pushvalue(L, -1);
      lua_setglobal(L, name);
   }

   return 1;
}

void lutro_checked_stack_assert(lua_State* L, int expectedTop, char const* file, int line)
{
   // TODO: report this error once per file/line, as a means of reducing spam potential
   //  (requires some hash table, perhaps stored within the lua_State itself)

   int curTop = lua_gettop(L);
   fflush(stdout);
   fprintf(stderr, "%s:%d: Stack assertion failed: got=%i expected=%i.\n", file, line, curTop, expectedTop);
   fflush(stderr);
   lutro_stack_dump(L);
   fflush(stdout);

   // recovery: set the expected stack on exit. App can usually continue running.
   // (TODO: add some option for fast-fail assertion here, such as a debug breakpoint or such, but such a thing
   //  usually needs to be ignorable to be useful)
   lua_settop(L, expectedTop); 
}

void lutro_stack_dump(lua_State* L)
{
   int i;
   int top = lua_gettop(L);

   printf("   %4s | %4s | %16s | %10s | %s\n",
          "Abs", "Rel", "Addr", "Type", "Value");
   puts("   -----------------------------------------------------");

   for (i = top; i >= 1; i--)
   {
      int t = lua_type(L, i);
      printf("   %4i | %4i | %16p | %10s | ",
             i, i - top - 1, lua_topointer(L, i), lua_typename(L, t));

      switch (t) {
      case LUA_TSTRING:
         printf("'%s'\n", lua_tostring(L, i));
         break;
      case LUA_TBOOLEAN:
         printf("%s\n",lua_toboolean(L, i) ? "true" : "false");
         break;
      case LUA_TNUMBER:
         printf("%g\n", lua_tonumber(L, i));
         break;
      case LUA_TTABLE:
         puts( "table" );
         break;
       case LUA_TFUNCTION:
         puts( "function" );
         break;
       case LUA_TUSERDATA:
         puts( "userdata" );
         break;
       case LUA_TTHREAD:
         puts( "thread" );
         break;
       case LUA_TLIGHTUSERDATA:
         puts( "ltuserdata" );
         break;
       default:
         puts("");
         break;
      }
   }
   printf("\n");
}

int lutro_require(lua_State *L, const char *modname, int pop_result)
{
   lua_getglobal(L, "require");
   lua_pushstring(L, modname);

   int success = lutro_pcall(L, 1, 1) == 0;

   if (success && pop_result)
      lua_pop(L, 1);

   return success;
}

void lutro_relpath_to_modname(char *outmod, const char *relpath)
{
   int i;
   int len = strlen(relpath);

   strlcpy(outmod, relpath, len + 1);

   path_remove_extension(outmod);

   for (i = 0; i < len; ++i)
   {
      char c = outmod[i];

      if (c == '/')
         c = '.';

      outmod[i] = c;
   }
}

/**
 * Retrieves the current version.
 *
 * https://love2d.org/wiki/love.getVersion
 */
int lutro_getVersion(lua_State *L) {
   lua_pushnumber(L, VERSION_MAJOR);
   lua_pushnumber(L, VERSION_MINOR);
   lua_pushnumber(L, VERSION_PATCH);
   lua_pushstring(L, "Lutro");

   return 4;
}

#if LUA_VERSION_NUM < 502

#ifndef G
#define G(L)	(L->l_G)
#endif
#define api_check2(l,e,msg)	luai_apicheck(l,(e) && msg)

#ifndef HAVE_JIT
LUA_API const lua_Number *lua_version (lua_State *L)
{
   static const lua_Number version = LUA_VERSION_NUM;

//   if (L == NULL) return &version;
//   else return G(L)->version;
   return &version;
}
#endif

#define luaL_checkversion(L)	luaL_checkversion_(L, LUA_VERSION_NUM)
LUALIB_API void luaL_checkversion_ (lua_State *L, lua_Number ver)
{
   const lua_Number *v = lua_version(L);
   if (v != lua_version(NULL))
      luaL_error(L, "multiple Lua VMs detected");
   else if (*v != ver)
      luaL_error(L, "version mismatch: app. needs %f, Lua core provides %f",
                 ver, *v);
   lua_pushnumber(L, -(lua_Number)0x1234);
   if (lua_tointeger(L, -1) != -0x1234 /*||
       lua_tounsigned(L, -1) != (unsigned int)-0x1234*/)
      luaL_error(L, "bad conversion number->int;"
                    " must recompile Lua with proper settings");
   lua_pop(L, 1);
}

#ifndef HAVE_JIT
LUALIB_API void luaL_setfuncs (lua_State *L, const luaL_Reg *l, int nup)
{
   luaL_checkversion(L);
   luaL_checkstack(L, nup, "too many upvalues");
   for (; l->name != NULL; l++)
   {
      int i;
      for (i = 0; i < nup; i++)
         lua_pushvalue(L, -nup);
      lua_pushcclosure(L, l->func, nup);
      lua_setfield(L, -(nup + 2), l->name);
   }
   lua_pop(L, nup);
}
#endif

LUA_API int lua_absindex (lua_State *L, int idx) {
   return (idx > 0 || idx <= LUA_REGISTRYINDEX)? idx : lua_gettop(L) + idx + 1;
}

LUA_API int lua_compare (lua_State *L, int index1, int index2, int op)
{
   int a = lua_absindex(L, index1);
   int b = lua_absindex(L, index2);
   int i = 0;

   if (!lua_isnil(L, a) && !lua_isnil(L, b)) {
      switch (op) {
         case LUA_OPEQ: i = lua_equal(L, a, b); break;
         case LUA_OPLT: i = lua_lessthan(L, a, b); break;
         case LUA_OPLE: i = lua_lessthan(L, a, b) || lua_equal(L, a, b); break;
         default: api_check2(L, 0, "invalid option");
      }
   }
   return i;
}

static int typeerror (lua_State *L, int narg, const char *tname)
{
   const char *msg = lua_pushfstring(L, "%s expected, got %s",
                                     tname, luaL_typename(L, narg));
   return luaL_argerror(L, narg, msg);
}


static void tag_error (lua_State *L, int narg, int tag)
{
   typeerror(L, narg, lua_typename(L, tag));
}

LUALIB_API unsigned luaL_checkunsigned (lua_State *L, int narg)
{
   if (!lua_isnumber(L, narg))
      tag_error(L, narg, LUA_TNUMBER);
   return lua_tointeger(L, narg);
}

#endif

int l_not_implemented(lua_State *L)
{
   lua_pop(L, lua_gettop(L));
   return luaL_error(L, "Not implemented.");
}

int luax_insistglobal(lua_State *L, const char *k)
{
   lua_getglobal(L, k);

   if (!lua_istable(L, -1))
   {
      lua_pop(L, 1); // Pop the non-table.
      lua_newtable(L);
      lua_pushvalue(L, -1);
      lua_setglobal(L, k);
   }

   return 1;
}

int luax_c_insistglobal(lua_State *L, const char *k)
{
   return luax_insistglobal(L, k);
}

void luax_register(lua_State *L, const char *name, const luaL_Reg *l)
{
   if (name)
      lua_newtable(L);

   luax_setfuncs(L, l);
   if (name)
   {
      lua_pushvalue(L, -1);
      lua_setglobal(L, name);
   }
}

void luax_setfuncs(lua_State *L, const luaL_Reg *l)
{
   if (!l)
      return;

   for (; l->name; l++)
   {
      lua_pushcfunction(L, l->func);
      lua_setfield(L, -2, l->name);
   }
}
