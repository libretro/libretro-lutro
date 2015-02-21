#include "lutro.h"
#include "runtime.h"
#include "file/file_path.h"
#include "compat/strl.h"
#include "unzip.h"

#include "graphics.h"
#include "input.h"
#include "audio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>
#include <libgen.h>
#include <unistd.h>

static lua_State *L;
lutro_settings_t settings = {
   .width = 320,
   .height = 240,
   .pitch = 0,
   .framebuffer = NULL,
   .input_cb = NULL
};

static int lutro_core_preload(lua_State *L)
{
   lutro_ensure_global_table(L, "lutro");

   return 1;
}

static void init_settings(lua_State *L)
{
   lutro_ensure_global_table(L, "lutro");

   lua_newtable(L);

   lua_pushnumber(L, settings.width);
   lua_setfield(L, -2, "width");

   lua_pushnumber(L, settings.height);
   lua_setfield(L, -2, "height");

   lua_setfield(L, -2, "settings");

   lua_pop(L, 1);
}

void lutro_init()
{
   L = luaL_newstate();
   luaL_openlibs(L);

   init_settings(L);

   lutro_preload(L, lutro_core_preload, "lutro");
   lutro_preload(L, lutro_graphics_preload, "lutro.graphics");
   lutro_preload(L, lutro_audio_preload, "lutro.audio");
   lutro_preload(L, lutro_input_preload, "lutro.input");

   lua_getglobal(L, "require");
   lua_pushstring(L, "lutro");
   lua_call(L, 1, 1);

   lua_getglobal(L, "require");
   lua_pushstring(L, "lutro.graphics");
   lua_call(L, 1, 1);

   lua_getglobal(L, "require");
   lua_pushstring(L, "lutro.audio");
   lua_call(L, 1, 1);

   lua_getglobal(L, "require");
   lua_pushstring(L, "lutro.input");
   lua_call(L, 1, 1);

   // remove this if undefined references to lutro.* happen
   lua_pop(L, 2);
}

void lutro_deinit()
{
   lua_close(L);
}

int lutro_set_package_path(lua_State* L, const char* path)
{
   const char *cur_path;
   char new_path[PATH_MAX_LENGTH];
   lua_getglobal(L, "package");
   lua_getfield(L, -1, "path");
   cur_path = lua_tostring( L, -1);
   strlcpy(new_path, cur_path, sizeof(new_path));
   strlcat(new_path, path, sizeof(new_path));
   lua_pop(L, 1);
   lua_pushstring(L, new_path) ;
   lua_setfield(L, -2, "path");
   lua_pop(L, 1);
   return 1;
}

int lutro_unzip(const char *path, const char *extraction_directory)
{
   path_mkdir(extraction_directory);

   unzFile *zipfile = unzOpen(path);
   if ( zipfile == NULL )
   {
      printf("%s: not found\n", path);
      return -1;
   }

   unz_global_info global_info;
   if (unzGetGlobalInfo(zipfile, &global_info) != UNZ_OK)
   {
      printf("could not read file global info\n");
      unzClose(zipfile);
      return -1;
   }

   char read_buffer[8192];

   uLong i;
   for (i = 0; i < global_info.number_entry; ++i)
   {
      unz_file_info file_info;
      char filename[PATH_MAX_LENGTH];
      if (unzGetCurrentFileInfo(zipfile, &file_info, filename, PATH_MAX_LENGTH,
         NULL, 0, NULL, 0 ) != UNZ_OK)
      {
         printf( "could not read file info\n" );
         unzClose( zipfile );
         return -1;
      }

      const size_t filename_length = strlen(filename);
      if (filename[filename_length-1] == '/')
      {
         printf("dir:%s\n", filename);
         char abs_path[PATH_MAX_LENGTH];
         fill_pathname_join(abs_path,
               extraction_directory, filename, sizeof(abs_path));
         path_mkdir(abs_path);
      }
      else
      {
         printf("file:%s\n", filename);
         if (unzOpenCurrentFile(zipfile) != UNZ_OK)
         {
            printf("could not open file\n");
            unzClose(zipfile);
            return -1;
         }

         char abs_path[PATH_MAX_LENGTH];
         fill_pathname_join(abs_path,
               extraction_directory, filename, sizeof(abs_path));
         FILE *out = fopen(abs_path, "wb");
         if (out == NULL)
         {
            printf("could not open destination file\n");
            unzCloseCurrentFile(zipfile);
            unzClose(zipfile);
            return -1;
         }

         int error = UNZ_OK;
         do
         {
            error = unzReadCurrentFile(zipfile, read_buffer, 8192);
            if (error < 0)
            {
               printf("error %d\n", error);
               unzCloseCurrentFile(zipfile);
               unzClose(zipfile);
               return -1;
            }

            if (error > 0)
               fwrite(read_buffer, error, 1, out);

         } while (error > 0);

         fclose(out);
      }

      unzCloseCurrentFile(zipfile);

      if (i + 1  < global_info.number_entry)
      {
         if (unzGoToNextFile(zipfile) != UNZ_OK)
         {
            printf("cound not read next file\n");
            unzClose(zipfile);
            return -1;
         }
      }
   }

   unzClose(zipfile);
   return 0;
}

int lutro_load(const char *path)
{
   if (!strcmp(path_get_extension(path), "lutro"))
   {
      char extr_dir[PATH_MAX_LENGTH];
      fill_pathname(extr_dir, path, "", sizeof(extr_dir));
      lutro_unzip(path, extr_dir);
      fill_pathname_join(path,
            extr_dir, "main.lua", PATH_MAX_LENGTH*sizeof(char));
   }

   char package_path[PATH_MAX_LENGTH];
   strlcpy(package_path, ";", sizeof(package_path));
   strlcat(package_path, path, sizeof(package_path));
   path_basedir(package_path);
   strlcat(package_path, "?.lua;", sizeof(package_path));
   lutro_set_package_path(L, package_path);

   if(luaL_dofile(L, path))
   {
       fprintf(stderr, "%s\n", lua_tostring(L, -1));
       lua_pop(L, 1);

       return 0;
   }

   lua_getglobal(L, "lutro");

   char game_dir[PATH_MAX_LENGTH];
   strlcpy(game_dir, path, sizeof(game_dir));
   path_basedir(game_dir);
   lua_pushstring(L, game_dir);
   lua_setfield(L, -2, "path");

   lua_pushnumber(L, 0);
   lua_setfield(L, -2, "camera_x");

   lua_pushnumber(L, 0);
   lua_setfield(L, -2, "camera_y");

   lua_getfield(L, -1, "conf");

   if (lua_isnoneornil(L, -1))
   {
      puts("skipping custom configuration.");
   }
   else
   {
      lua_getfield(L, -2, "settings");

      if(lua_pcall(L, 1, 0, 0))
      {
         fprintf(stderr, "%s\n", lua_tostring(L, -1));
         lua_pop(L, 1);

         return 0;
      }

      lua_getfield(L, -1, "settings");

      lua_getfield(L, -1, "width");
      settings.width = lua_tointeger(L, -1);
      lua_remove(L, -1);

      lua_getfield(L, -1, "height");
      settings.height = lua_tointeger(L, -1);
      lua_remove(L, -1);
   }

   lua_pop(L, 1); // either lutro.settings or lutro.conf

   lutro_graphics_init();

   lua_getfield(L, -1, "load");

   if (lua_isnoneornil(L, -1))
   {
      puts("skipping custom initialization.");
   }
   else
   {
      if(lua_pcall(L, 0, 0, 0))
      {
         fprintf(stderr, "%s\n", lua_tostring(L, -1));
         lua_pop(L, 1);

         return 0;
      }
   }

   lua_pop(L, 1);

   return 1;
}

int lutro_run(double delta)
{
   lua_getglobal(L, "lutro");

   lua_getfield(L, -1, "update");

   if (lua_isfunction(L, -1))
   {
      lua_pushnumber(L, delta);
      if(lua_pcall(L, 1, 0, 0))
      {
         fprintf(stderr, "%s\n", lua_tostring(L, -1));
         lua_pop(L, 1);

         return 0;
      }
   } else {
      lua_pop(L, 1);
   }

   lua_getfield(L, -1, "draw");
   if (lua_isfunction(L, -1))
   {
      if(lua_pcall(L, 0, 0, 0))
      {
         fprintf(stderr, "%s\n", lua_tostring(L, -1));
         lua_pop(L, 1);

         return 0;
      }
   } else {
      lua_pop(L, 1);
   }

   lua_pop(L, 1);

   return 1;
}
