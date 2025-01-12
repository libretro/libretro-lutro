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
#include <streams/file_stream.h>
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
static int32_t allocation_count = 0;

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

// Like lua_isfunction, but this one pops the item off the stack if it's not a function.
// Intended for use when wrapping lutro_pcall() only. This function should not be used in
// situations where a polymorphic argument is being tested for multiple possible valid types.
// returns a boolean result, TRUE (non-zero) if the index on the stack is a function.
int lutro_pcall_isfunction(lua_State* L, int idx) {
   if (!lua_isfunction(L, -1))
   {
      lua_pop(L, 1);
      return 0;
   }

   return 1;
}

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

// prints error and backtrace to console if an error occurs during the call. The error is left on
// on the stack and should be removed with lua_pop() if this function returns non-zero.
int lutro_pcall(lua_State *L, int narg, int nret)
{
   int base = lua_gettop(L) - narg;
   lua_pushcfunction(L, traceback);
   lua_insert(L, base);  // move traceback below the arguments
   int result = lua_pcall(L, narg, nret, base);
   lua_remove(L, base);  // remove traceback function
   return result;
}

int lutro_pcall_cached_traceback(lua_State *L, int narg, int nret, int traceback)
{
   return lua_pcall(L, narg, nret, traceback);
}

static int dofile(lua_State *L, const char *path)
{
   int res;

   res = luaL_loadfile(L, path);
   if (res)
      return res;

   res = lutro_pcall(L, 0, LUA_MULTRET);
   return res;
}

static void init_lutro_global_table(lua_State *L)
{
   lua_getglobal(L, "lutro");

   if (!lua_istable(L, -1)) {
      lua_pop(L, 1);
      lua_newtable(L);

      // Introduce lutro.getVersion().
      lua_pushcfunction(L, lutro_getVersion);
      lua_setfield(L, -2, "getVersion");

      // Add the "lutro" Lua global.
      lua_pushvalue(L, -1);
      lua_setglobal(L, "lutro");
   }

   lua_pop(L, 1);
}


static int lutro_core_preload(lua_State *L)
{
   return 1;
}

static void init_settings(lua_State *L)
{
   player_checked_stack_begin(L);
   luax_reqglobal(L, "lutro");

   lua_newtable(L);
   lua_pushnumber(L, settings.width);
   lua_setfield(L, -2, "width");

   lua_pushnumber(L, settings.height);
   lua_setfield(L, -2, "height");

   lua_setfield(L, -2, "settings");    // lutro.settings
   lua_pop(L, 1);
   player_checked_stack_end(L, 0);
}

void lutro_newlib_x(lua_State* L, luaL_Reg const* funcs, char const* fieldname, int numfuncs)
{
   luax_reqglobal(L, "lutro");
   lua_createtable(L, 0, numfuncs);
   luaL_setfuncs(L, funcs, 0);
   lua_setfield(L, -2, fieldname);
   lua_pop(L, 1);
}

void lutro_init()
{
   L = luaL_newstate();

   // handler for errors that occur outside pcall() scope
   lua_atpanic(L, &lutro_lua_panic);

   luaL_openlibs(L);

   // impose default behavior that ensures stdout prints in realtime and is in correct
   // chronological sequence with stderr.
   luaL_dostring(L, "io.stdout:setvbuf('no')");

#ifdef HAVE_JIT
   luaJIT_setmode(L, -1, LUAJIT_MODE_WRAPCFUNC|LUAJIT_MODE_ON);
#endif

   player_checked_stack_begin(L);

   init_lutro_global_table(L);
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

   player_checked_stack_end(L, 0);
}

void lutro_deinit()
{
#ifdef HAVE_INOTIFY
   if (settings.live_enable)
      lutro_live_deinit();
#endif

   lutro_audio_stop_all(L);
   mixer_unref_stopped_sounds(L);
   lua_gc(L, LUA_GCSTEP, 0);
   lua_close(L);

   lutro_audio_deinit();
   lutro_filesystem_deinit();

   lutro_print_allocation();

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
   luax_reqglobal(L, "package");
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

   if (!path_is_directory(mainfile)) {
      path_basedir(gamedir);
   }

   // Loading a .lutro file.
   if (!strcmp(path_get_extension(mainfile), "lutro"))
   {
      fill_pathname(gamedir, mainfile, "/", sizeof(gamedir));
      fill_pathname(gamedir, conffile, "/", sizeof(gamedir));
      lutro_unzip(mainfile, gamedir);
   }

   fill_pathname_join(mainfile, gamedir, "main.lua", sizeof(mainfile));
   if (!filestream_exists(mainfile))
      fill_pathname_join(mainfile, gamedir, "main.luac", sizeof(mainfile));

   fill_pathname_join(conffile, gamedir, "conf.lua", sizeof(conffile));
   if (!filestream_exists(conffile))
      fill_pathname_join(conffile, gamedir, "conf.luac", sizeof(conffile));

   fill_pathname_slash(gamedir, sizeof(gamedir));

   char package_path[PATH_MAX_LENGTH];
   snprintf(package_path, PATH_MAX_LENGTH, ";%s?.lua;%s?.luac;%s?/init.lua", gamedir, gamedir, gamedir);
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
   luax_reqglobal(L, "lutro");
   int tbl_top_lutro = lua_gettop(L);

   strlcpy(settings.gamedir, gamedir, PATH_MAX_LENGTH);

   lua_getfield(L, tbl_top_lutro, "conf");

   // Process the custom configuration, if it exists.
   if (lutro_pcall_isfunction(L, -1))
   {
      player_checked_stack_begin(L);
      lua_getfield(L, tbl_top_lutro, "settings");

      if(lutro_pcall(L, 1, 0))
      {
         lua_pop(L, 1);
         return 0;
      }

      lua_getfield(L, tbl_top_lutro, "settings");

      lua_getfield(L, -1, "width");
      lua_getfield(L, -2, "height");
      lua_getfield(L, -3, "live_enable");
      lua_getfield(L, -4, "live_call_load");

      settings.width          = lua_tointeger(L, -4);
      settings.height         = lua_tointeger(L, -3);
      settings.live_enable    = lua_toboolean(L, -2);
      settings.live_call_load = lua_toboolean(L, -1);

      lua_pop(L, 4);
      player_checked_stack_end(L, 0);
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

   lua_getfield(L, tbl_top_lutro, "load");

   int result = 1;
   // Check if lutro.load() exists.
   if (lutro_pcall_isfunction(L, -1))
   {
      // It exists, so call lutro.load().
      if(lutro_pcall(L, 0, 0))
      {
         lua_pop(L, 1);
         result = 0;
      }
   }

   lua_settop(L, oldtop);
   return 1;
}

void lutro_gamepadevent(lua_State* L)
{
   tool_checked_stack_begin(L);

   luax_reqglobal(L, "lutro");

   unsigned i;
   for (i = 0; i < 16; i++)
   {
      int16_t is_down = settings.input_cb(0, RETRO_DEVICE_JOYPAD, 0, i);
      if (is_down != input_cache[i])
      {
         lua_getfield(L, -1, is_down ? "gamepadpressed" : "gamepadreleased");
         if (lutro_pcall_isfunction(L, -1))
         {
            lua_pushnumber(L, i);
            lua_pushstring(L, input_find_name(joystick_enum, i));
            if (lutro_pcall(L, 2, 0)) // takes care of poping the function too
            {
               lua_pop(L, 1);
            }
            input_cache[i] = is_down;
         }
      }
   }
   lua_pop(L, 1);
   tool_checked_stack_end(L, 0);
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

   player_checked_stack_begin(L);
   lua_pushcfunction(L, traceback);
   int idx_traceback = lua_gettop(L);
   luax_reqglobal(L, "lutro");

   lua_getfield(L, -1, "update");   // lutro["update"]
   if (lutro_pcall_isfunction(L, -1))
   {
      lua_pushnumber(L, delta);

      if(lutro_pcall_cached_traceback(L, 1, 0, idx_traceback))
      {
         lua_pop(L, 1);
      }
   }

   lua_getfield(L, -1, "draw");     // lutro["draw"]
   if (lutro_pcall_isfunction(L, -1))
   {
      lutro_graphics_begin_frame(L);

      if(lutro_pcall_cached_traceback(L, 0, 0, idx_traceback))
      {
         lua_pop(L, 1);
      }
      lutro_graphics_end_frame(L);
   }

   lua_pop(L,2);     // lutro table and traceback

   lutro_keyboardevent(L);
   lutro_gamepadevent(L);
   lutro_mouseevent(L);
   lutro_joystickevent(L);

   player_checked_stack_end(L,0);

   mixer_unref_stopped_sounds(L);
   lua_gc(L, LUA_GCSTEP, 0);
}

void lutro_reset()
{
   player_checked_stack_begin(L);
   luax_reqglobal(L, "lutro");
   lua_getfield(L, -1, "reset");

   if (lutro_pcall_isfunction(L, -1))
   {
      lutro_audio_stop_all(L);
      if(lutro_pcall(L, 0, 0))
      {
         lua_pop(L, 1);
      }
   }

   player_checked_stack_end(L,0);
   lua_gc(L, LUA_GCSTEP, 0);
}

size_t lutro_serialize_size()
{
   size_t size = 0;

   player_checked_stack_begin(L);
   luax_reqglobal(L, "lutro");
   lua_getfield(L, -1, "serializeSize");

   if (lutro_pcall_isfunction(L, -1))
   {
      if (lutro_pcall(L, 0, 1))
      {
         lua_pop(L, 1);
      }

      if (lua_isnumber(L, -1))
         size = lua_tonumber(L, -1);
      else
         tool_assertf(false, "Invalid type returned from lutro.serializeSize. An integer result is expected.\n");
   }

   player_checked_stack_end(L,0);
   lua_gc(L, LUA_GCSTEP, 0);

   return size;
}

bool lutro_serialize(void *data_, size_t size)
{
   player_checked_stack_begin(L);
   luax_reqglobal(L, "lutro");
   lua_getfield(L, -1, "serialize");

   if (lutro_pcall_isfunction(L, -1))
   {
      lua_pushnumber(L, size);
      if (lutro_pcall(L, 1, 1))
      {
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

   player_checked_stack_end(L,0);
   lua_gc(L, LUA_GCSTEP, 0);

   return true;
}

bool lutro_unserialize(const void *data_, size_t size)
{
   player_checked_stack_begin(L);
   luax_reqglobal(L, "lutro");
   lua_getfield(L, -1, "unserialize");

   if (lutro_pcall_isfunction(L, -1))
   {
      lua_pushstring(L, data_);
      lua_pushnumber(L, size);
      if (lutro_pcall(L, 2, 0))
      {
         lua_pop(L, 1);
      }
   }

   player_checked_stack_end(L,0);
   lua_gc(L, LUA_GCSTEP, 0);

   return true;
}

void lutro_cheat_set(unsigned index, bool enabled, const char *code)
{
   player_checked_stack_begin(L);
   luax_reqglobal(L, "lutro");
   lua_getfield(L, -1, "cheat_set");

   if (lutro_pcall_isfunction(L, -1))
   {
      lua_pushnumber(L, index);
      lua_pushboolean(L, enabled);
      lua_pushstring(L, code);
      if (lutro_pcall(L, 3, 0))
      {
         lua_pop(L, 1);
      }
   }

   player_checked_stack_end(L,0);
   lua_gc(L, LUA_GCSTEP, 0);
}

void lutro_cheat_reset()
{
   player_checked_stack_begin(L);
   luax_reqglobal(L, "lutro");
   lua_getfield(L, -1, "cheat_reset");

   if (lutro_pcall_isfunction(L, -1))
   {
      if (lutro_pcall(L, 0, 0))
      {
         lua_pop(L, 1);
      }
   }

   player_checked_stack_end(L,0);
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

void *lutro_malloc_internal(size_t size, const char* debug, int line)
{
    void *a = malloc(size);
#if TRACE_ALLOCATION
    if (a) {
        fprintf(stderr,"TRACE ALLOC:%p:malloc:%s:%d\n", a, debug, line);
        allocation_count++;
    } else {
        fprintf(stderr,"TRACE ALLOC:failure:malloc:%s:%d\n", debug, line);
    }
#endif
    return a;
}

void lutro_free_internal(void *ptr, const char* debug, int line)
{
#if TRACE_ALLOCATION
    // Don't trace nop
    if (ptr) {
        fprintf(stderr,"TRACE ALLOC:%p:free:%s:%d\n", ptr, debug, line);
        allocation_count--;
    }
#endif
    free(ptr);
}

void *lutro_calloc_internal(size_t nmemb, size_t size, const char* debug, int line)
{
    void *a = calloc(nmemb, size);
#if TRACE_ALLOCATION
    if (a) {
        fprintf(stderr,"TRACE ALLOC:%p:calloc:%s:%d\n", a, debug, line);
        allocation_count++;
    } else {
        fprintf(stderr,"TRACE ALLOC:failure:calloc:%s:%d\n", debug, line);
    }
#endif
    return a;
}

void *lutro_realloc_internal(void *ptr, size_t size, const char* debug, int line)
{
    void *a = realloc(ptr, size);
#if TRACE_ALLOCATION
    if (a) {
        if (ptr == NULL) {
            // If original pointer is null, realloc behave as a malloc
            fprintf(stderr,"TRACE ALLOC:%p:realloc (malloc):%s:%d\n", a, debug, line);
            // Note even if size is 0, realloc can return a pointer suitable to
            // be passed to free
            allocation_count++;
        } else {
            fprintf(stderr,"TRACE ALLOC:%p:realloc (move from %p):%s:%d\n", a, ptr, debug, line);
        }
    } else {
        // Either realloc fail, or it released the memory
        if (ptr != NULL && size == 0) {
            // If size is 0, realloc behave as a free
            fprintf(stderr,"TRACE ALLOC:null:realloc (free %p):%s:%d\n", ptr, debug, line);
            allocation_count--;
        } else {
            fprintf(stderr,"TRACE ALLOC:failure:realloc (%p):%s:%d\n", ptr, debug, line);
        }
    }
#endif
    return a;
}

void lutro_print_allocation() {
#if TRACE_ALLOCATION
    fprintf(stderr,"TRACE ALLOC:total pending allocations:%d\n", allocation_count);
#endif
}
