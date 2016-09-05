#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "image.h"
#include "path.h"

#include "boot_lua.h"

char __cdecl *realpath( const char *__restrict__ name, char *__restrict__ resolved );

static int luamain( lua_State* L )
{
  int          argc = (int)lua_tonumber( L, lua_upvalueindex( 1 ) );
  const char** argv = (const char**)lua_touserdata( L, lua_upvalueindex( 2 ) );
  int          i;
  
  /* Load boot.lua */
  if ( luaL_loadbufferx( L, boot_lua, boot_lua_len, "boot.lua", "t" ) != LUA_OK )
  {
    return lua_error( L );
  }
  
  /* Run it, it returns the bootstrap main function */
  lua_call( L, 0, 1 );
  
  /* Prepare the args table */
  lua_newtable( L );
  
  for ( i = 1; i < argc; i++ )
  {
    lua_pushstring( L, argv[ i ] );
    lua_rawseti( L, -2, i - 1 );
  }
  
  /* Call the bootstrap main function with the args table */
  lua_call( L, 1, 1 );
  
  /* Returns the integer result */
  return (int)lua_tointeger( L, -1 );
}

static int traceback( lua_State* L )
{
  luaL_traceback( L, L, lua_tostring( L, -1 ), 1 );
  return 1;
}

int main( int argc, const char *argv[] ) 
{
  lua_State* L = luaL_newstate();
  int        top;
  
  if ( L )
  {
    top = lua_gettop( L );
    luaL_openlibs( L );
    luaL_requiref( L, LUA_IMAGELIBNAME, luaopen_image, 0 );
    luaL_requiref( L, LUA_PATHLIBNAME, luaopen_path, 0 );
    lua_settop( L, top );
    
    lua_pushcfunction( L, traceback );
    
    lua_pushnumber( L, argc );
    lua_pushlightuserdata( L, (void*)argv );
    lua_pushcclosure( L, luamain, 2 );
    
    if ( lua_pcall( L, 0, 1, -2 ) != 0 )
    {
      fprintf( stderr, "%s\n", lua_tostring( L, -1 ) );
    }
    
    top = (int)lua_tointeger( L, -1 );
    lua_close( L );
    return top;
  }
  
  fprintf( stderr, "could't create the Lua state\n" );
  return 1;
}
