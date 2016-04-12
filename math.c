
#include "lutro.h"
#include "math.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

void lutro_math_init()
{
    time_t t;
    srand((unsigned) time(&t));
}

int lutro_math_preload(lua_State *L)
{
   static luaL_Reg math_funcs[] = {
      { "random", lutro_math_random },
      { "setRandomSeed", lutro_math_setRandomSeed },
      {NULL, NULL}
   };

   lutro_ensure_global_table(L, "lutro");

   luaL_newlib(L, math_funcs);

   lua_setfield(L, -2, "math");

   return 1;
}

int lutro_math_random(lua_State *L)
{
    int n = lua_gettop(L);
    if (n > 2) {
        return luaL_error(L, "lutro.math.random requires 0, 1 or 2 arguments, %d given.", n);
    }

    double num = (double) rand() / (RAND_MAX);
    double min;
    double max;

    switch (n) {
        case 0:
            lua_pushnumber(L, num);
            break;
        case 1:
            max = (double) luaL_checknumber(L, 1);
            num *= max;
            lua_pushnumber(L, num);
            break;
        case 2:
            min = (double) luaL_checknumber(L, 1);
            max = (double) luaL_checknumber(L, 1);
            num *= max;
            num += min;
            lua_pushnumber(L, num);
            break;
    }

    return 1;
}

/**
 * lutro.math.setRandomSeed
 *
 * https://love2d.org/wiki/love.math.setRandomSeed
 */
int lutro_math_setRandomSeed(lua_State *L)
{
    int n = lua_gettop(L), arg1, arg2;
    if (n < 1 || n > 2) {
        return luaL_error(L, "lutro.math.setRandomSeed requires 1 or 2 arguments, %d given.", n);
    }

    switch (n) {
        case 1:
            arg1 = (unsigned) luaL_checknumber(L, 1);
            srand(arg1);
            break;
        case 2:
            // TODO: love.math.setRandomSeed expects to combine the two integers into one 64-bit integer.
            arg1 = (unsigned) luaL_checknumber(L, 1);
            arg2 = (unsigned) luaL_checknumber(L, 1);
            srand(arg1 + arg2);
            break;
    }

    return 0;
}
