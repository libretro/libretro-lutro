#include "runtime.h"
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

   printf("total in stack %d\n",top);

   for (i = 1; i <= top; i++)
   {  /* repeat for each level */
      int t = lua_type(L, i);

      printf("   %s: ", lua_typename(L, t));

      switch (t) {
      case LUA_TSTRING:  /* strings */
         printf("'%s'\n", lua_tostring(L, i));
         break;
      case LUA_TBOOLEAN:  /* booleans */
         printf("%s\n",lua_toboolean(L, i) ? "true" : "false");
         break;
      case LUA_TNUMBER:  /* numbers */
         printf("%g\n", lua_tonumber(L, i));
         break;
      default:  /* other values */
         printf("0x%x\n", lua_topointer(L, i));
         break;
      }
   }
   printf("\n");  /* end the listing */
}

