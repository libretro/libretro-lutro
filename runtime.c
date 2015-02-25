#include "runtime.h"
#include "compat/strl.h"
#include <assert.h>

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
      lua_pushvalue(L, -1);
      lua_setglobal(L, name);
   }

   return 1;
}

void lutro_namespace(lua_State *L)
{
   lutro_ensure_global_table(L, "lutro");
}

void lutro_stack_dump(lua_State* L)
{
   int i;
   int top = lua_gettop(L);

   printf("   %4s | %4s | %10s | %10s | %s\n",
          "Abs", "Rel", "Addr", "Type", "Value");
   puts("   ----------------------------------------------");

   for (i = top; i >= 1; i--)
   {
      int t = lua_type(L, i);
      printf("   %4i | %4i | %10p | %10s | ",
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

   int success = lua_pcall(L, 1, 1, 0) == LUA_OK;

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
