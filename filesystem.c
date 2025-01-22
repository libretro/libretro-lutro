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

int _raw_get_user_writable_dir(lua_State *L)
{
   char* savedir;
   if ((*settings.environ_cb)(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &savedir)) {
      lua_pushstring(L, savedir);
      return 1;
   }

   // in the case that no savedir is available: return nil rather than seeking a fallback.
   return 0;
}

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
      { "getAppdataDirectory", fs_getAppdataDirectory },
      { "isDirectory", fs_isDirectory },
      { "isFile",      fs_isFile },
      { "createDirectory", fs_createDirectory },
      { "getDirectoryItems", fs_getDirectoryItems },

      // internal helpers for lua-authored functions.
      { "_raw_get_user_writable_dir", _raw_get_user_writable_dir },
      {NULL, NULL}
   };

   lutro_newlib(L, fs_funcs, "filesystem");

   // lutro.filesystem._appendTrailingSlash - appends a trailing slash to match behavior of LOVE.
   // Because slash or backslash is platform dependent, this implementation takes the laziest
   // approach: if the incoming path has a trailing backslash then it checks if the OS environment
   // is Windows (WINDIR) and if yes then it doesn't append a trailing slash. It does not attempt
   // to append trialing backslashes. Windows tolerates forward slash and mixed-slash paths well.
   if (1) {
      int ret = luaL_dostring(L, "\
      lutro.filesystem._appendTrailingSlash = function(path)\n\
         if path:sub(-1) == '\\\\' then\n\
            local winDir = os.getenv('WINDIR')\n\
            if winDir ~= nil and winDir ~= '' then\n\
               path = path .. '/'\n\
            end\n\
         end\n\
         if path:sub(-1) ~= '/' then\n\
            path = path .. '/'\n\
         end\n\
         return path\n\
      end");

      if (ret) {
         // don't hard-fail - no need to block apps that might not use the function.
         lutro_errorf("failed to assign lutro.filesystem.getUserDirectory: %s\n", lua_tostring(L, -1));
         lua_pop(L, 1); // pop error message
      }
   }

   // lutro.filesystem.getUserDirectory
   // 
   // Returns the writable save directory provided by libretro frontend. The directory
   // may or may not be sandboxed according to a current user, depending on the design
   // and capabilities of the platform OS. Funtion may return nil if the libretro frontend
   // does not provide a writable save directory.
   //
   // implementation notes:
   //  - UserDirectory should always have a trailing slash (as confirmed on Love2D)
   //  - Cache the UserDir to avoid costly re-calculation of env var lookups. UserDir and all env vars
   //    are static for the lifetime of the process.
   //  - https://love2d.org/wiki/love.filesystem.getUserDirectory

   if (1) {
      int ret = luaL_dostring(L, "\
      lutro.filesystem.getUserDirectory = function()\n\
         local savedir = lutro.filesystem._raw_get_user_writable_dir()\n\
         if savedir == nil or savedir == '' then\n\
            return nil\n\
         end\n\
         return lutro.filesystem._appendTrailingSlash(savedir)\n\
      end");

      if (ret) {
         // don't hard-fail - no need to block apps that might not use the function.
         lutro_errorf("failed to assign lutro.filesystem.getUserDirectory: %s\n", lua_tostring(L, -1));
         lua_pop(L, 1); // pop error message
      }
   }

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
