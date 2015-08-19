#include "filesystem.h"
#include "lutro.h"
#include "compat/strl.h"
#include "file/file_path.h"

#include <stdlib.h>
#include <string.h>

int lutro_filesystem_preload(lua_State *L)
{
   static luaL_Reg gfx_funcs[] =  {
      { "exists",      fs_exists },
      { "read",        fs_read },
      { "setIdentity", fs_setIdentity },
      {NULL, NULL}
   };

   lutro_ensure_global_table(L, "lutro");

   luaL_newlib(L, gfx_funcs);

   lua_setfield(L, -2, "filesystem");

   return 1;
}

void lutro_filesystem_init()
{
}

int fs_read(lua_State *L)
{
   const char *path = luaL_checkstring(L, 1);

   char fullpath[PATH_MAX_LENGTH];
   strlcpy(fullpath, settings.gamedir, sizeof(fullpath));
   strlcat(fullpath, path, sizeof(fullpath));

   FILE *fp = fopen(fullpath, "r");
   if (!fp)
      return -1;

   fseek(fp, 0, SEEK_END);
   long fsize = ftell(fp);
   fseek(fp, 0, SEEK_SET);

   char *string = malloc(fsize + 1);
   fread(string, fsize, 1, fp);
   fclose(fp);

   string[fsize] = 0;

   lua_pushstring(L, string);

   return 1;
}

int fs_exists(lua_State *L)
{
   const char *path = luaL_checkstring(L, 1);

   char fullpath[PATH_MAX_LENGTH];
   strlcpy(fullpath, settings.gamedir, sizeof(fullpath));
   strlcat(fullpath, path, sizeof(fullpath));

   bool exists = path_file_exists(fullpath);

   lua_pushboolean(L, exists);

   return 1;
}

int fs_setIdentity(lua_State *L)
{
   const char *name = luaL_checkstring(L, 1);

   strlcpy(settings.identity, name, sizeof(settings.identity));

   return 0;
}
