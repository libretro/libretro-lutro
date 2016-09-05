#pragma once

#define LUA_PATHLIBNAME "path"

struct lua_State;

LUALIB_API int luaopen_path( lua_State* L );
