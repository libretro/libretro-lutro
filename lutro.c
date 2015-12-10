#include "lutro.h"
#include "runtime.h"
#include "file/file_path.h"
#include "compat/strl.h"
#include "unzip.h"

#include "image.h"
#include "graphics.h"
#include "input.h"
#include "audio.h"
#include "sound.h"
#include "filesystem.h"
#include "system.h"
#include "timer.h"
#include "window.h"
#include "live.h"

#ifdef HAVE_JIT
#include "luajit.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>
#ifndef __CELLOS_LV2__
#include <libgen.h>
#endif
#include <unistd.h>

static lua_State *L;
static int16_t input_cache[16];

lutro_settings_t settings = {
   .width = 320,
   .height = 240,
   .pitch = 0,
   .framebuffer = NULL,
   .live_enable = 0,
   .live_call_load = 0,
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

#ifdef HAVE_JIT
   luaJIT_setmode(L, -1, LUAJIT_MODE_WRAPCFUNC|LUAJIT_MODE_ON);
#endif

   lutro_checked_stack_begin();

   init_settings(L);

   lutro_preload(L, lutro_core_preload, "lutro");
   lutro_preload(L, lutro_image_preload, "lutro.image");
   lutro_preload(L, lutro_graphics_preload, "lutro.graphics");
   lutro_preload(L, lutro_audio_preload, "lutro.audio");
   lutro_preload(L, lutro_sound_preload, "lutro.sound");
   lutro_preload(L, lutro_input_preload, "lutro.input");
   lutro_preload(L, lutro_filesystem_preload, "lutro.filesystem");
   lutro_preload(L, lutro_system_preload, "lutro.system");
   lutro_preload(L, lutro_timer_preload, "lutro.timer");
   lutro_preload(L, lutro_window_preload, "lutro.window");
#ifdef HAVE_INOTIFY
   lutro_preload(L, lutro_live_preload, "lutro.live");
#endif

   // if any of these requires fail, the checked stack assertion at the end will
   // be triggered. remember that assertions are only avaialable in debug mode.
   lutro_require(L, "lutro", 1);
   lutro_require(L, "lutro.image", 1);
   lutro_require(L, "lutro.graphics", 1);
   lutro_require(L, "lutro.audio", 1);
   lutro_require(L, "lutro.sound", 1);
   lutro_require(L, "lutro.input", 1);
   lutro_require(L, "lutro.filesystem", 1);
   lutro_require(L, "lutro.system", 1);
   lutro_require(L, "lutro.timer", 1);
   lutro_require(L, "lutro.window", 1);
#ifdef HAVE_INOTIFY
   lutro_require(L, "lutro.live", 1);
#endif

   lutro_checked_stack_assert(0);
}

void lutro_deinit()
{
#ifdef HAVE_INOTIFY
   if (settings.live_enable)
      lutro_live_deinit();
#endif

   lua_close(L);

   /* Has to be deinitialized later. */
   lutro_audio_deinit();
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
   char mainfile[PATH_MAX_LENGTH];
   char gamedir[PATH_MAX_LENGTH];

   strlcpy(mainfile, path, PATH_MAX_LENGTH);
   strlcpy(gamedir, path, PATH_MAX_LENGTH);

   if (path_is_directory(mainfile))
      fill_pathname_join(mainfile, gamedir, "main.lua", sizeof(mainfile));
   else
      path_basedir(gamedir);

   if (!strcmp(path_get_extension(mainfile), "lutro"))
   {
      fill_pathname(gamedir, mainfile, "/", sizeof(gamedir));
      lutro_unzip(mainfile, gamedir);
      fill_pathname_join(mainfile, gamedir, "main.lua", sizeof(mainfile));
   }

   fill_pathname_slash(gamedir, sizeof(gamedir));

   char package_path[PATH_MAX_LENGTH];
   snprintf(package_path, PATH_MAX_LENGTH, ";%s?.lua;%s?/init.lua", gamedir, gamedir);
   lutro_set_package_path(L, package_path);

   if(luaL_dofile(L, mainfile))
   {
       fprintf(stderr, "%s\n", lua_tostring(L, -1));
       lua_pop(L, 1);

       return 0;
   }

   lua_getglobal(L, "lutro");

   strlcpy(settings.gamedir, gamedir, PATH_MAX_LENGTH);

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

      lua_getfield(L, -1, "live_enable");
      settings.live_enable = lua_toboolean(L, -1);
      lua_remove(L, -1);

      lua_getfield(L, -1, "live_call_load");
      settings.live_call_load = lua_toboolean(L, -1);
      lua_remove(L, -1);
   }

   lua_pop(L, 1); // either lutro.settings or lutro.conf

   lutro_graphics_init();
   lutro_audio_init();

#ifdef HAVE_INOTIFY
   if (settings.live_enable)
      lutro_live_init();
#endif

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

void lutro_gamepadevent(lua_State* L)
{
   unsigned i;
   for (i = 0; i < 16; i++)
   {
      int16_t is_down = settings.input_cb(0, RETRO_DEVICE_JOYPAD, 0, i);
      if (is_down != input_cache[i])
      {
         lua_getfield(L, -1, is_down ? "gamepadpressed" : "gamepadreleased");
         if (lua_isfunction(L, -1))
         {
            lua_pushnumber(L, i);
            lua_pushstring(L, input_find_name(joystick_enum, i));
            if (lua_pcall(L, 2, 0, 0))
            {
               fprintf(stderr, "%s\n", lua_tostring(L, -1));
               lua_pop(L, 1);
            }
            input_cache[i] = is_down;
         }
         else
         {
            lua_pop(L, 1);
         }
      }
   }
}

void lutro_run(double delta)
{
#ifdef HAVE_INOTIFY
   if (settings.live_enable)
      lutro_live_update(L);
#endif

   lua_getglobal(L, "lutro");

   lua_getfield(L, -1, "update");

   if (lua_isfunction(L, -1))
   {
      lua_pushnumber(L, delta);
      if(lua_pcall(L, 1, 0, 0))
      {
         fprintf(stderr, "%s\n", lua_tostring(L, -1));
         lua_pop(L, 1);
      }
   } else {
      lua_pop(L, 1);
   }

   lua_getfield(L, -1, "draw");
   if (lua_isfunction(L, -1))
   {
      lutro_graphics_begin_frame(L);
      if(lua_pcall(L, 0, 0, 0))
      {
         fprintf(stderr, "%s\n", lua_tostring(L, -1));
         lua_pop(L, 1);
      }
      lutro_graphics_end_frame(L);
   } else {
      lua_pop(L, 1);
   }

   lutro_gamepadevent(L);

   lua_pop(L, 1);

   lua_gc(L, LUA_GCSTEP, 0);
}
