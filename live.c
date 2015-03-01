#include "live.h"
#include "lutro.h"
#include "compat/strl.h"
#include "file/file_path.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>

static struct {
   int ifd; // inotify fd
   int wfd; // watch fd
} live;

int lutro_live_preload(lua_State *L)
{
   static luaL_Reg funcs[] =  {
      {NULL, NULL}
   };

   lutro_ensure_global_table(L, "lutro");
   luaL_newlib(L, funcs);

   lua_setfield(L, -2, "live");

   return 1;
}

void lutro_live_init()
{
   memset(&live, 0, sizeof(live));

   live.ifd = inotify_init1(IN_NONBLOCK);

   if (live.ifd < 0)
   {
      perror("Failed to initialize inotify");
      lutro_live_deinit();
      return;
   }

   // XXX: Some editors do not trigger IN_MODIFY since they write to a temp file
   //      and rename() it to the actual file.
   live.wfd = inotify_add_watch(live.ifd, settings.gamedir, IN_MODIFY|IN_MOVED_TO);
   if (live.wfd < 0)
   {
      perror("Failed to monitor game directory");
      lutro_live_deinit();
      return;
   }
}

void lutro_live_deinit()
{
   settings.live_enable = 0;

   if (live.wfd >= 0)
      close(live.wfd);

   if (live.ifd >= 0)
      close(live.ifd);
}

// copies stuff from table dst to table src
static inline void shallow_update(lua_State *L, int src, int dst)
{
   lutro_checked_stack_begin();

   assert(lua_istable(L, src));
   assert(lua_istable(L, dst));

   src = lua_absindex(L, src);
   dst = lua_absindex(L, dst);

   lua_pushnil(L);
   while (lua_next(L, src))
   {
      lua_pushvalue(L, -2);
      lua_insert(L, -2);
      lua_settable(L, dst);
   }

   lutro_checked_stack_assert(0);
}

// expects value to be on the top of the stack. it'll be pop'd out
void set_package_loaded(lua_State *L, const char *modname)
{
   lua_getglobal(L, "package");
   lua_getfield(L, -1, "loaded");
   lua_pushvalue(L, -3);
   lua_setfield(L, -2, modname);
   lua_pop(L, 3);
}

void get_package_loaded(lua_State *L, const char *modname)
{
   lua_getglobal(L, "package");
   lua_getfield(L, -1, "loaded");
   lua_getfield(L, -1, modname);
   lua_remove(L, -2);
   lua_remove(L, -2);
}

void deep_update_once(lua_State *L, int src, int dst, int updated)
{
   assert(lua_istable(L, src));
   assert(lua_istable(L, dst));
   assert(lua_istable(L, updated));

   src = lua_absindex(L, src);
   dst = lua_absindex(L, dst);
   updated = lua_absindex(L, updated);

   lutro_checked_stack_begin();

   lua_pushvalue(L, dst);
   lua_gettable(L, updated);

   if (lua_isnoneornil(L, -1))
   {
      lua_pop(L, 1);

      lua_pushvalue(L, dst);
      lua_pushboolean(L, 1);
      lua_settable(L, updated);

      int top = lua_gettop(L);
      int src_mt = lua_getmetatable(L, src);
      int dst_mt = lua_getmetatable(L, dst);
      if (src_mt && dst_mt)
         deep_update_once(L, src_mt, dst_mt, updated);

      lua_pop(L, lua_gettop(L) - top);

      lua_pushnil(L);
      while (lua_next(L, src))
      {
         if (lua_istable(L, -1))
         {
            lua_pushvalue(L, -2);
            lua_gettable(L, dst);

            deep_update_once(L, -2, -1, updated);
            lua_pop(L, 2);
         }
         else
         {
            lua_pushvalue(L, -2);
            lua_insert(L, -2);
            lua_settable(L, dst);
         }
      }
   }
   else
      lua_pop(L, 1);

   lutro_checked_stack_assert(0);
}

static void update_inner_tables_once(lua_State *L, int src, int dst, int updated)
{
   lutro_checked_stack_begin();

   assert(lua_istable(L, src));
   assert(lua_istable(L, dst));
   assert(lua_istable(L, updated));

   src = lua_absindex(L, src);
   dst = lua_absindex(L, dst);
   updated = lua_absindex(L, updated);

   lua_pushnil(L);
   while (lua_next(L, src))
   {
      lua_pushvalue(L, -2);
      lua_gettable(L, dst);

      if (!lua_compare(L, -2, -1, LUA_OPEQ) && lua_istable(L, -2))
      {
         deep_update_once(L, -1, -2, updated);
         lua_pushvalue(L, -3);
         lua_pushvalue(L, -2);
         lua_settable(L, dst);
      }

      lua_pop(L, 2);
   }

   lutro_checked_stack_assert(0);
}

// TODO: check if there's anything else that needs to be backed up
static int live_hotswap(lua_State *L, const char *filename)
{
   char modname[PATH_MAX_LENGTH];
   int success = 1;

   lutro_checked_stack_begin();

   // backup _G then drop it
   lua_pushglobaltable(L);
   lua_newtable(L);
   shallow_update(L, -2, -1);
   lua_remove(L, -2);

   lutro_relpath_to_modname(modname, filename);

   // backup module state
   get_package_loaded(L, modname);

   // force reloading of the module
   lua_pushnil(L);
   set_package_loaded(L, modname);

   if(!lutro_require(L, modname, 0))
   {
      fprintf(stderr, "lutro.live lua error: %s\n", lua_tostring(L, -1));
      lua_pop(L, 1);

      lua_pushglobaltable(L);
      shallow_update(L, -3, -1);
      lua_pop(L, 1); // error and backups

      success = 0;
   }
   else
   {
      lua_newtable(L); // to keep track of updated keys

      if (lua_istable(L, -3))
         deep_update_once(L, -2, -3, -1);

      // drop newmod
      lua_remove(L, -2);

      // restore _G then drop it
      lua_pushglobaltable(L);
      update_inner_tables_once(L, -4, -1, -2);
      lua_pop(L, 1);

      lua_pop(L, 1); // drop updated key table
   }

   // restore oldmod
   set_package_loaded(L, modname);

   lua_pop(L, 1); // _G backup

   if (success && settings.live_call_load)
   {
      lua_getglobal(L, "lutro");
      lua_getfield(L, -1, "load");

      // assume load is defined.
      if(lua_pcall(L, 0, 0, 0))
      {
         // TODO: dump stacktrace
         fprintf(stderr, "%s\n", lua_tostring(L, -1));
         lua_pop(L, 1);
      }
      lua_pop(L, 1);
   }

   lutro_checked_stack_assert(0);

   return success;
}

void lutro_live_update(lua_State *L)
{
   char buf[sizeof(struct inotify_event) + PATH_MAX_LENGTH + 1]; // this ought to be enough for a single event
   int rdsize = 0;
   int swapped = 0;

   // read all events
   while ((rdsize = read(live.ifd, buf, sizeof(buf))) >= 0)
   {
      const char *ptr;
      const struct inotify_event *ev;

      for (ptr = buf; ptr < buf + rdsize; ptr += sizeof(struct inotify_event) + ev->len)
      {
         ev = (const struct inotify_event*)ptr;

         if (ev->len && strcmp(path_get_extension(ev->name), "lua") == 0)
         {
            clock_t start = clock();
            if (live_hotswap(L, ev->name))
            {
               printf("Swapped %s in %.3fs\n", ev->name,
                      ((double)clock() - (double)start)* 1.0e-6);
               swapped = 1;
            }
            else
               printf("Swapping of %s failed.\n", ev->name);
         }
      }
   }

   if (errno != EAGAIN)
   {
      perror("Could not read inotify event");
      return;
   }

   if (swapped)
      lua_gc(L, LUA_GCCOLLECT, 0);
}


void lutro_live_draw()
{

}
