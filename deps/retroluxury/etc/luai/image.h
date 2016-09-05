#pragma once

#define LUA_IMAGELIBNAME "image"

struct lua_State;

LUALIB_API int luaopen_image( lua_State* L );
