#include "filesystem.h"
#include "lutro.h"
#include <compat/strl.h>
#include <file/file_path.h>
#include <streams/file_stream.h>
#include <vfs/vfs_implementation.h>
#include <string/stdstring.h>

#if WANT_PHYSFS
#include "physfs.h"
#endif

#include <stdlib.h>
#include <string.h>

int lutro_filesystem_preload(lua_State *L)
{
   static const luaL_Reg fs_funcs[] =  {
      { "exists",      fs_exists },
      { "read",        fs_read },
      { "write",       fs_write },
      { "setRequirePath", fs_setRequirePath },
      { "getRequirePath", fs_getRequirePath },
      { "load",        fs_load },
      { "setIdentity", fs_setIdentity },
      { "getUserDirectory", fs_getUserDirectory },
      { "getAppdataDirectory", fs_getAppdataDirectory },
      { "isDirectory", fs_isDirectory },
      { "isFile",      fs_isFile },
      { "createDirectory", fs_createDirectory },
      { "getDirectoryItems", fs_getDirectoryItems },
      {NULL, NULL}
   };

   lutro_newlib(L, fs_funcs, "filesystem");

   return 1;
}

void lutro_filesystem_init()
{
   #if WANT_PHYSFS
   PHYSFS_init(NULL);
   #endif
}

void lutro_filesystem_deinit()
{
   #if WANT_PHYSFS
   PHYSFS_deinit();
   #endif
}

/**
 * contents, size = lutro.filesystem.read(name, size)
 *
 * https://love2d.org/wiki/love.filesystem.read
 */
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

   char *string      = lutro_malloc(fsize + 1);
   size_t bytes_read = fread(string, 1, fsize, fp);
   fclose(fp);

   string[bytes_read] = 0;

   lua_pushstring(L, string);
   lua_pushnumber(L, bytes_read);

   lutro_free(string);

   return 2;
}

int fs_write(lua_State *L)
{
   const char *path = luaL_checkstring(L, 1);
   const char *data = luaL_checkstring(L, 2);

   char fullpath[PATH_MAX_LENGTH];
   strlcpy(fullpath, settings.gamedir, sizeof(fullpath));
   strlcat(fullpath, path, sizeof(fullpath));

   FILE *fp = fopen(fullpath, "w");
   if (!fp)
      return -1;

   fprintf(fp, "%s", data);

   fclose(fp);

   lua_pushboolean(L, 1);
   return 1;
}

/**
 * lutro.filesystem.setRequirePath
 *
 * @see https://love2d.org/wiki/love.filesystem.setRequirePath
 */
int fs_setRequirePath(lua_State *L)
{
   const char *path = luaL_checkstring(L, 1);
   lua_getglobal(L, "package");
   lua_pushstring(L, path) ;
   lua_setfield(L, -2, "path");
   lua_pop(L, 1);

   return 0;
}

/**
 * lutro.filesystem.getRequirePath
 *
 * @see https://love2d.org/wiki/love.filesystem.getRequirePath
 */
int fs_getRequirePath(lua_State *L)
{
   const char *cur_path;
   lua_getglobal(L, "package");
   lua_getfield(L, -1, "path");
   cur_path = lua_tostring( L, -1);
   lua_pop(L, 1);

   lua_pushstring(L, cur_path);

   return 1;
}

int fs_load(lua_State *L)
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

   char *string = lutro_malloc(fsize + 1);
   fread(string, fsize, 1, fp);
   fclose(fp);

   string[fsize] = 0;

   int status = luaL_loadbuffer(L, string, fsize, path);

   lutro_free(string);

   switch (status)
   {
   case LUA_ERRMEM:
      return luaL_error(L, "Memory allocation error: %s\n", lua_tostring(L, -1));
   case LUA_ERRSYNTAX:
      return luaL_error(L, "Syntax error: %s\n", lua_tostring(L, -1));
   default:
      return 1;
   }
}

int fs_exists(lua_State *L)
{
   const char *path = luaL_checkstring(L, 1);

   char fullpath[PATH_MAX_LENGTH];
   strlcpy(fullpath, settings.gamedir, sizeof(fullpath));
   strlcat(fullpath, path, sizeof(fullpath));

   bool exists = filestream_exists(fullpath);

   lua_pushboolean(L, exists);
   return 1;
}

int fs_setIdentity(lua_State *L)
{
   const char *name = luaL_checkstring(L, 1);

   strlcpy(settings.identity, name, sizeof(settings.identity));

   return 0;
}

/**
 * lutro.filesystem.getUserDirectory
 *
 * https://love2d.org/wiki/love.filesystem.getUserDirectory
 */
int fs_getUserDirectory(lua_State *L)
{
   // Retrieve the user's home directory from environment variables.
   char *homedir = getenv("HOME");
   if (homedir == NULL) {
      homedir = getenv("HOMEDRIVE");
      if (homedir == NULL) {
         // TODO: Figure out what to do when HOME isn't available.
         homedir = ".";
      }
   }

   // If needed, append a / at the end of the homedir.
   size_t len = strlen(homedir);
   if (homedir[len] != '/') {
      homedir[len++] = '/';
      homedir[len] = '\0';
   }

   lua_pushstring(L, homedir);

   return 1;
}

/**
 * lutro.filesystem.getAppdataDirectory
 *
 * Retrieves libretro's SYSTEM directory.
 *
 * @see https://love2d.org/wiki/love.filesystem.getAppdataDirectory
 * @see RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY
 */
int fs_getAppdataDirectory(lua_State *L)
{
   char* appdataDir;
   if ((*settings.environ_cb)(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &appdataDir)) {
      lua_pushstring(L, appdataDir);
   }
   else {
      lua_pushstring(L, "");
   }
   return 1;
}

int fs_isDirectory(lua_State *L)
{
   const char *path = luaL_checkstring(L, 1);

   char fullpath[PATH_MAX_LENGTH];
   strlcpy(fullpath, settings.gamedir, sizeof(fullpath));
   strlcat(fullpath, path, sizeof(fullpath));

   bool res = path_is_directory(fullpath);

   lua_pushboolean(L, res);
   return 1;
}

int fs_isFile(lua_State *L)
{
   char fullpath[PATH_MAX_LENGTH];
   bool res         = false;
   const char *path = luaL_checkstring(L, 1);

   strlcpy(fullpath, settings.gamedir, sizeof(fullpath));
   strlcat(fullpath, path, sizeof(fullpath));

   res = filestream_exists(fullpath) && !path_is_directory(fullpath);

   lua_pushboolean(L, res);
   return 1;
}

int fs_createDirectory(lua_State *L)
{
   bool res;
   char fullpath[PATH_MAX_LENGTH];
   const char *path = luaL_checkstring(L, 1);
   strlcpy(fullpath, settings.gamedir, sizeof(fullpath));
   strlcat(fullpath, path, sizeof(fullpath));

   res = path_mkdir(fullpath);

   lua_pushboolean(L, res);
   return 1;
}

int fs_getDirectoryItems(lua_State *L)
{
   // Validate number of arguments.
   int n = lua_gettop(L);
   if (n != 1) {
      return luaL_error(L, "lutro.filesystem.getDirectoryItems requires 1 argument, %d given.", n);
   }

   // Get the full resolved path to the desired directory.
   const char *path = luaL_checkstring(L, 1);
   char fullpath[PATH_MAX_LENGTH];
   strlcpy(fullpath, settings.gamedir, sizeof(fullpath));
   strlcat(fullpath, path, sizeof(fullpath));

   // Make sure it's a directory.
   if (!path_is_directory(fullpath)) {
      return luaL_error(L, "The given directory of '%s' is not a directory.", path);
   }

   // Open up the directory.
   libretro_vfs_implementation_dir* dir = retro_vfs_opendir_impl(fullpath, true);
   if (!dir) {
      retro_vfs_closedir_impl(dir);
      return luaL_error(L, "Failed to open the '%s' directory.", path);
   }

   // Prepare the output table.
   lua_newtable(L);
   int index = 0;

   // Iterate through each directory entry, ignoring the current and previous directories.
   while (retro_vfs_readdir_impl(dir)) {
      const char * currentDir = retro_vfs_dirent_get_name_impl(dir);
      if (currentDir == NULL) {
         break;
      }
      if (string_is_equal(currentDir, ".") || string_is_equal(currentDir, "..")) {
         continue;
      }

      // Add the entry to the table.
      lua_pushnumber(L, index++);
      lua_pushstring(L, currentDir);
      lua_settable(L, -3);
   }

   // Finally, close the opened directory, and return the table.
   retro_vfs_closedir_impl(dir);

   return 1;
}
