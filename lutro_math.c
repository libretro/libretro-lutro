#include "lutro.h"
#include "lutro_math.h"

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
   static const luaL_Reg math_funcs[] = {
      { "random", lutro_math_random },
      { "setRandomSeed", lutro_math_setRandomSeed },
      {NULL, NULL}
   };

   lutro_newlib(L, math_funcs, "math");

   return 1;
}

/**
 * rand()/RAND_MAX can only return RAND_MAX possible values
 * inlcuding 0 and 1, seemingly skewing the odds in their favor
 *
 * rand() % x usually slightly favors smaller numbers
 */
int lutro_math_random(lua_State *L)
{
    int n = lua_gettop(L);
    if (n > 2) {
        return luaL_error(L, "lutro.math.random requires 0, 1 or 2 arguments, %d given.", n);
    }

    int num = rand();
    double num0;
    int min;
    int max;

    switch (n) {
        case 0:
            num0 = (double) num / (RAND_MAX);
            lua_pushnumber(L, num0);
            break;
        case 1:
            max = luaL_checknumber(L, 1);
            num = num % max + 1;
            lua_pushnumber(L, num);
            break;
        case 2:
            min = luaL_checknumber(L, 1);
            max = luaL_checknumber(L, 2);
            if (min > max) {
                min ^= max;
                max ^= min;
                min ^= max;
            }
            num = num % (max-min+1);
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
            arg2 = (unsigned) luaL_checknumber(L, 2);
            srand(arg1 + arg2);
            break;
    }

    return 0;
}
