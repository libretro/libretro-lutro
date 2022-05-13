#include "lutro.h"
#include "runtime.h"
#include "unzip.h"

#include "image.h"
#include "graphics.h"
#include "input.h"
#include "audio.h"
#include "event.h"
#include "keyboard.h"
#include "sound.h"
#include "filesystem.h"
#include "system.h"
#include "timer.h"
#include "lutro_math.h"
#include "lutro_window.h"
#include "live.h"
#include "mouse.h"
#include "joystick.h"

#include <file/file_path.h>
#include <compat/strl.h>
#include <ctype.h>

#ifdef HAVE_JIT
#include "luajit.h"
#endif

// LuaUTF8
#include "deps/luautf8/lutf8lib.h"

// LuaSocket
#ifdef HAVE_LUASOCKET
#include "deps/luasocket/luasocket.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>

#if !defined(_MSC_VER)
#ifndef __CELLOS_LV2__
#include <libgen.h>
#endif
#include <unistd.h>
#endif

static lua_State *L;
static int16_t input_cache[16];
int g_lua_stack = 0;

lutro_settings_t settings = {
   .width = 320,
   .height = 240,
   .pitch = 0,
   .framebuffer = NULL,
   .live_enable = 0,
   .live_call_load = 0,
   .input_cb = NULL,
   .delta = 0,
   .deltaCounter = 0,
   .frameCounter = 0,
   .fps = 0
};

struct retro_perf_callback perf_cb;

#if 0
static void dumpstack( lua_State* L )
{
  int top = lua_gettop( L );

  for ( int i = 1; i <= top; i++ )
  {
    printf( "%2d %3d ", i, i - top - 1 );

    lua_pushvalue( L, i );

    switch ( lua_type( L, -1 ) )
    {
    case LUA_TNIL:
      printf( "nil\n" );
      break;
    case LUA_TNUMBER:
      printf( "%e\n", lua_tonumber( L, -1 ) );
      break;
    case LUA_TBOOLEAN:
      printf( "%s\n", lua_toboolean( L, -1 ) ? "true" : "false" );
      break;
    case LUA_TSTRING:
      printf( "\"%s\"\n", lua_tostring( L, -1 ) );
      break;
    case LUA_TTABLE:
      printf( "table\n" );
      break;
    case LUA_TFUNCTION:
      printf( "function\n" );
      break;
    case LUA_TUSERDATA:
      printf( "userdata\n" );
      break;
    case LUA_TTHREAD:
      printf( "thread\n" );
      break;
    case LUA_TLIGHTUSERDATA:
      printf( "light userdata\n" );
      break;
    default:
      printf( "?\n" );
      break;
    }
  }

  lua_settop( L, top );
}
#endif

int _lutro_assertf_internal(int ignorable, const char *fmt, ...)
{
   fflush(NULL);

   va_list argptr;
   va_start(argptr, fmt);
   vfprintf(stderr, fmt, argptr);
   va_end(argptr);

   // tips: the fmt input should always be in the predefined format of:
   //   FILE(LINE): assertion `cond` failed.
   // example:
   //   lutro.cpp(444): assertion `x > 0` failed.
   //
   // We can use this knowledge to parse the file and line positions and perform additional clever filtering
   // or log prep/routing.

   int top = lua_gettop(L);
   lua_getglobal(L, "debug");
   lua_getfield(L, -1, "traceback");
   lua_pushstring(L, "");
   lua_pushinteger(L, 2);
   lua_call(L, 2, 1);

   fflush(NULL);
   const char* msg = lua_tostring(L, -1);
   while (*msg == '\r' || *msg == '\n') ++msg;  // lua pads some newlines at the beginning.. strip 'em
   fprintf(stderr, "%s\n", msg);
   lua_pop(L, 1);

   if (ignorable)
   {
      // TODO : suspend the core, show up some user dialog via libretro api?
      // (it could even have lots of info, like lua stacktrace... )
      // return 0 if ignored.
   }

   return 1;
}

static int lutro_lua_panic (lua_State *L) {
   // currently this aborts, but it is also possible to set up a setjmp/longmp handler for
   // these, as the vast majority of them _are_ recoverable, in the sense that we can usually
   // bounce all the way out to retro_run and continue executing some unrelated system.
   // For example, if keyboard or pad or audio APIs panic, this doesn't have to  stop video,
   // or other portions of the engine, fron continuing on (usually).

   // If implementing longjmp, it would be nice to change this into a play_assert (ignorable).

   fprintf(stderr, "lua_panic!\n%s\n", lua_tostring(L, -1));
   abort();
}

int traceback(lua_State *L) {
   // use lua's provided debug.traceback to get a contextual error.
   lua_getglobal(L, "debug");
   lua_getfield(L, -1, "traceback");
   lua_pushvalue(L, 1);
   lua_pushinteger(L, 2);
   lua_call(L, 2, 1);

   // generally we don't want to stop/assert on lua errors. The majority are ignorable/recoverable
   // and the better strategy is to surface the info the the content creator via some interface
   // that's slightly more friendly than a console window. (eg, something that captures all the spam
   // and produces a summary report of unique errors)
   //tool_errorf("%s\n", lua_tostring(L, -1));

   fflush(NULL);
   fprintf(stderr, "%s\n", lua_tostring(L, -1));

   return 1;
}

int lutro_pcall(lua_State *L, int narg, int nret)
{
   int handler = lua_gettop(L) - narg - 1;
   while (handler && (lua_tocfunction(L, handler) != traceback)) --handler;
   dbg_assert(lua_tocfunction(L, handler) == traceback);
   return lua_pcall(L, narg, nret, handler);
}

static int dofile(lua_State *L, const char *path)
{
   int res;

   lua_pushcfunction(L, traceback);
   res = luaL_loadfile(L, path);
   if (res)
      return res;

   res = lutro_pcall(L, 0, LUA_MULTRET);
   return res;
}

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

   // handler for errors thatr occur outside pcall() scope
   lua_atpanic(L, &lutro_lua_panic);

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
   lutro_preload(L, lutro_event_preload, "lutro.event");
   lutro_preload(L, lutro_sound_preload, "lutro.sound");
   lutro_preload(L, lutro_input_preload, "lutro.input");
   lutro_preload(L, lutro_filesystem_preload, "lutro.filesystem");
   lutro_preload(L, lutro_keyboard_preload, "lutro.keyboard");
   lutro_preload(L, lutro_system_preload, "lutro.system");
   lutro_preload(L, lutro_timer_preload, "lutro.timer");
   lutro_preload(L, lutro_math_preload, "lutro.math");
   lutro_preload(L, lutro_window_preload, "lutro.window");
   lutro_preload(L, lutro_mouse_preload, "lutro.mouse");
   lutro_preload(L, lutro_joystick_preload, "lutro.joystick");

   // UTF8
   lutro_preload(L, luaopen_luautf8, "utf8");

   // LuaSocket
#ifdef HAVE_LUASOCKET
   _o_open(L);
#endif

#ifdef HAVE_INOTIFY
   lutro_preload(L, lutro_live_preload, "lutro.live");
#endif

   // if any of these requires fail, the checked stack assertion at the end will
   // be triggered. remember that assertions are only avaialable in debug mode.
   lutro_require(L, "lutro", 1);
   lutro_require(L, "lutro.image", 1);
   lutro_require(L, "lutro.graphics", 1);
   lutro_require(L, "lutro.audio", 1);
   lutro_require(L, "lutro.event", 1);
   lutro_require(L, "lutro.sound", 1);
   lutro_require(L, "lutro.keyboard", 1);
   lutro_require(L, "lutro.input", 1);
   lutro_require(L, "lutro.filesystem", 1);
   lutro_require(L, "lutro.system", 1);
   lutro_require(L, "lutro.timer", 1);
   lutro_require(L, "lutro.math", 1);
   lutro_require(L, "lutro.window", 1);
   lutro_require(L, "lutro.mouse", 1);
   lutro_require(L, "lutro.joystick", 1);
#ifdef HAVE_INOTIFY
   lutro_require(L, "lutro.live", 1);
#endif

   // Mirror the lutro namespace to "love".
   luaL_dostring(L, "love = lutro");

   // Initialize the filesystem.
   lutro_filesystem_init();

   lutro_checked_stack_assert(0);
}

void lutro_deinit()
{
#ifdef HAVE_INOTIFY
   if (settings.live_enable)
      lutro_live_deinit();
#endif

   lutro_audio_stop_all(L);
   lua_gc(L, LUA_GCSTEP, 0);
   lua_close(L);

   lutro_audio_deinit();
   lutro_filesystem_deinit();
}

void lutro_mixer_render(int16_t* buffer)
{
   if (!L) return;
   mixer_render(L, buffer);
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
         //printf("dir:%s\n", filename);
         char abs_path[PATH_MAX_LENGTH];
         fill_pathname_join(abs_path,
               extraction_directory, filename, sizeof(abs_path));
         path_mkdir(abs_path);
      }
      else
      {
         //printf("file:%s\n", filename);
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
   // conf.lua https://love2d.org/wiki/Config_Files
   char conffile[PATH_MAX_LENGTH];
   char gamedir[PATH_MAX_LENGTH];

   strlcpy(mainfile, path, PATH_MAX_LENGTH);
   strlcpy(conffile, path, PATH_MAX_LENGTH);
   strlcpy(gamedir, path, PATH_MAX_LENGTH);

   if (path_is_directory(mainfile)) {
      fill_pathname_join(mainfile, gamedir, "main.lua", sizeof(mainfile));
      fill_pathname_join(conffile, gamedir, "conf.lua", sizeof(conffile));
   }
   else {
      path_basedir(gamedir);
   }

   // Loading a .lutro file.
   if (!strcmp(path_get_extension(mainfile), "lutro"))
   {
      fill_pathname(gamedir, mainfile, "/", sizeof(gamedir));
      fill_pathname(gamedir, conffile, "/", sizeof(gamedir));
      lutro_unzip(mainfile, gamedir);
      fill_pathname_join(mainfile, gamedir, "main.lua", sizeof(mainfile));
      fill_pathname_join(conffile, gamedir, "conf.lua", sizeof(conffile));
   }
   else {
      // Loading a main.lua file, so construct the config file.
      fill_pathname_join(conffile, gamedir, "conf.lua", sizeof(conffile));
   }

   fill_pathname_slash(gamedir, sizeof(gamedir));

   char package_path[PATH_MAX_LENGTH];
   snprintf(package_path, PATH_MAX_LENGTH, ";%s?.lua;%s?/init.lua", gamedir, gamedir);
   lutro_set_package_path(L, package_path);

   // Load the configuration file, ignoring any errors.
   dofile(L, conffile);

   // Now that configuration is in place, load main.lua.
   if (dofile(L, mainfile))
   {
       fprintf(stderr, "%s\n", lua_tostring(L, -1));
       lua_pop(L, 1);

       return 0;
   }

   int oldtop = lua_gettop(L);
   lua_pushcfunction(L, traceback);
   lua_getglobal(L, "lutro");

   int tbl_top_lutro = lua_gettop(L);

   strlcpy(settings.gamedir, gamedir, PATH_MAX_LENGTH);

   lua_getfield(L, -1, "conf");

   // Process the custom configuration, if it exists.
   if (lua_isfunction(L, -1))
   {
      lua_getfield(L, -2, "settings");

      if(lutro_pcall(L, 1, 0))
      {
         fprintf(stderr, "%s\n", lua_tostring(L, -1));
         lua_pop(L, 1);

         return 0;
      }

      // no stack cleanup necessary inside oldtop scope.

      lua_getfield(L, -1, "settings");

      lua_getfield(L, -1, "width");
      lua_getfield(L, -2, "height");
      lua_getfield(L, -3, "live_enable");
      lua_getfield(L, -4, "live_call_load");

      settings.width          = lua_tointeger(L, -4);
      settings.height         = lua_tointeger(L, -3);
      settings.live_enable    = lua_toboolean(L, -2);
      settings.live_call_load = lua_toboolean(L, -1);
   }

   lutro_graphics_init(L);
   lutro_audio_init(L);
   lutro_event_init();
   lutro_math_init();
   lutro_joystick_init();

#ifdef HAVE_INOTIFY
   if (settings.live_enable)
      lutro_live_init();
#endif
   lua_settop(L, tbl_top_lutro);
   lua_getfield(L, -1, "load");

   int result = 1;
   // Check if lutro.load() exists.
   if (lua_isfunction(L, -1))
   {
      // It exists, so call lutro.load().
      if(lutro_pcall(L, 0, 0))
      {
         fprintf(stderr, "%s\n", lua_tostring(L, -1));
         lua_pop(L, 1);
         result = 0;
      }
   }

   lua_settop(L, oldtop);
   return 1;
}

void lutro_gamepadevent(lua_State* L)
{
   ENTER_LUA_STACK
   unsigned i;

   for (i = 0; i < 16; i++)
   {
      int16_t is_down = settings.input_cb(0, RETRO_DEVICE_JOYPAD, 0, i);
      if (is_down != input_cache[i])
      {
         lua_getglobal(L, "lutro");
         lua_getfield(L, -1, is_down ? "gamepadpressed" : "gamepadreleased");
         if (lua_isfunction(L, -1))
         {
            lua_pushnumber(L, i);
            lua_pushstring(L, input_find_name(joystick_enum, i));
            if (lutro_pcall(L, 2, 0)) // takes care of poping the function too
            {
               fprintf(stderr, "%s\n", lua_tostring(L, -1));
               lua_pop(L, 1);
            }
            input_cache[i] = is_down;
         }
         else
         {
            lua_pop(L, 1); // pop getfield gamepadpressed or gamepadreleased
         }
         lua_pop(L, 1); // pop getglobal lutro
      }
   }
   EXIT_LUA_STACK
}

void lutro_run(double delta)
{
   // Update the Delta and FPS.
   settings.delta = delta;
   settings.deltaCounter += delta;
   settings.frameCounter += 1;
   settings.fps = 1 / delta;

   if (settings.deltaCounter >= 1.0) {
      settings.frameCounter = 0;
      settings.deltaCounter = 0;
   }

#ifdef HAVE_INOTIFY
   if (settings.live_enable)
      lutro_live_update(L);
#endif

   int oldtop = lua_gettop(L);
   lua_pushcfunction(L, traceback);

   lua_getglobal(L, "lutro");
   lua_getfield(L, -1, "update");
   if (lua_isfunction(L, -1))
   {
      lua_pushnumber(L, delta);

      if(lutro_pcall(L, 1, 0))
      {
         fprintf(stderr, "%s\n", lua_tostring(L, -1));
         lua_pop(L, 1);
      }
   }

   lua_getfield(L, -1, "draw");
   if (lua_isfunction(L, -1))
   {
      lutro_graphics_begin_frame(L);

      if(lutro_pcall(L, 0, 0))
      {
         fprintf(stderr, "%s\n", lua_tostring(L, -1));
         lua_pop(L, 1);
      }
      lutro_graphics_end_frame(L);
   }

   lutro_keyboardevent(L);
   lutro_gamepadevent(L);
   lutro_mouseevent(L);
   lutro_joystickevent(L);

   lua_settop(L, oldtop);

   mixer_unref_stopped_sounds(L);
   lua_gc(L, LUA_GCSTEP, 0);
}

void lutro_reset()
{
   int oldtop = lua_gettop(L);

   lua_pushcfunction(L, traceback);

   lua_getglobal(L, "lutro");
   lua_getfield(L, -1, "reset");

   if (lua_isfunction(L, -1))
   {
      lutro_audio_stop_all(L);
      if(lutro_pcall(L, 0, 0))
      {
         fprintf(stderr, "%s\n", lua_tostring(L, -1));
         lua_pop(L, 1);
      }
   }

   lua_settop(L, oldtop);
   lua_gc(L, LUA_GCSTEP, 0);
}

size_t lutro_serialize_size()
{
   size_t size = 0;
   int oldtop = lua_gettop(L);

   lua_pushcfunction(L, traceback);

   lua_getglobal(L, "lutro");
   lua_getfield(L, -1, "serializeSize");

   if (lua_isfunction(L, -1))
   {
      if (lutro_pcall(L, 0, 1))
      {
         fprintf(stderr, "%s\n", lua_tostring(L, -1));
         lua_pop(L, 1);
      }

      if (lua_isnumber(L, -1))
         size = lua_tonumber(L, -1);
      else
         tool_assertf(false, "Invalid type returned from lutro.serializeSize. An integer result is expected.\n");
   }

   lua_settop(L, oldtop);
   lua_gc(L, LUA_GCSTEP, 0);

   return size;
}

bool lutro_serialize(void *data_, size_t size)
{
   int oldtop = lua_gettop(L);
   lua_pushcfunction(L, traceback);

   lua_getglobal(L, "lutro");
   lua_getfield(L, -1, "serialize");

   if (lua_isfunction(L, -1))
   {
      lua_pushnumber(L, size);
      if (lutro_pcall(L, 1, 1))
      {
         fprintf(stderr, "%s\n", lua_tostring(L, -1));
         lua_pop(L, 1);
      }
      else
      {
         const char* data = lua_tostring(L, -1);
         lua_pop(L, 1);

         memset(data_, 0, size);
         memcpy(data_, data, strlen(data));
      }
   }

   lua_settop(L, oldtop);
   lua_gc(L, LUA_GCSTEP, 0);

   return true;
}

bool lutro_unserialize(const void *data_, size_t size)
{
   int oldtop = lua_gettop(L);
   lua_pushcfunction(L, traceback);

   lua_getglobal(L, "lutro");
   lua_getfield(L, -1, "unserialize");

   if (lua_isfunction(L, -1))
   {
      lua_pushstring(L, data_);
      lua_pushnumber(L, size);
      if (lutro_pcall(L, 2, 0))
      {
         fprintf(stderr, "%s\n", lua_tostring(L, -1));
         lua_pop(L, 1);
      }
   }

   lua_settop(L, oldtop);
   lua_gc(L, LUA_GCSTEP, 0);

   return true;
}

void lutro_cheat_set(unsigned index, bool enabled, const char *code)
{
   int oldtop = lua_gettop(L);
   lua_pushcfunction(L, traceback);

   lua_getglobal(L, "lutro");
   lua_getfield(L, -1, "cheat_set");

   if (lua_isfunction(L, -1))
   {
      lua_pushnumber(L, index);
      lua_pushboolean(L, enabled);
      lua_pushstring(L, code);
      if (lutro_pcall(L, 3, 0))
      {
         fprintf(stderr, "%s\n", lua_tostring(L, -1));
         lua_pop(L, 1);
      }
   }

   lua_settop(L, oldtop);
   lua_gc(L, LUA_GCSTEP, 0);
}

void lutro_cheat_reset()
{
   int oldtop = lua_gettop(L);
   lua_pushcfunction(L, traceback);

   lua_getglobal(L, "lutro");
   lua_getfield(L, -1, "cheat_reset");

   if (lua_isfunction(L, -1))
   {
      if (lutro_pcall(L, 0, 0))
      {
         fprintf(stderr, "%s\n", lua_tostring(L, -1));
         lua_pop(L, 1);
      }
   }

   lua_settop(L, oldtop);
   lua_gc(L, LUA_GCSTEP, 0);
}

void lutro_assetPath_init(AssetPathInfo* dest, const char* path)
{
   assert (dest);

   strlcpy(dest->fullpath, settings.gamedir, sizeof(dest->fullpath));
   strlcat(dest->fullpath, path, sizeof(dest->fullpath));

   //get file extension
   strcpy(dest->ext, path_get_extension(path));
   for(int i = 0; dest->ext[i]; i++)
      dest->ext[i] = tolower((uint8_t)dest->ext[i]);
}
