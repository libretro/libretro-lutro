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
   live.wfd = inotify_add_watch(live.ifd, settings.mainfile, IN_MODIFY);
   if (live.wfd < 0)
   {
      perror("Failed to monitor main.lua");
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

// TODO: check if there's anything else that needs to be backed up
static void live_hotswap(lua_State *L, const char *filename)
{
   char modname[PATH_MAX_LENGTH];

   int backup = 0;

   lutro_checked_stack_begin();

   // backup _G then drop it
   lua_pushglobaltable(L);
   lua_newtable(L);
   shallow_update(L, -2, -1);
   lua_remove(L, -2);

   backup = lua_absindex(L, -1);

   lutro_relpath_to_modname(modname, filename);

   lua_pushnil(L);
   set_package_loaded(L, modname);

   if(!lutro_require(L, modname, 1))
   {
      fprintf(stderr, "lutro.live lua error: %s\n", lua_tostring(L, -1));
      lua_pop(L, 2); // error and backup
   }
   else
   {
      // restore _G then drop it and the backup
      lua_pushglobaltable(L);
      shallow_update(L, backup, -1);
      lua_pop(L, 2);

      if (settings.live_call_load)
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
   }

   lutro_checked_stack_assert(0);
}

void lutro_live_update(lua_State *L)
{
   char buf[sizeof(struct inotify_event) + PATH_MAX_LENGTH + 1]; // this ought to be enough for a single event
   int modified = 0;

   // read all events
   while (read(live.ifd, buf, sizeof(buf)) >= 0)
   {
      modified = 1;
   }

   if (errno != EAGAIN)
   {
      perror("Could not read inotify event");
      return;
   }

   if (modified)
      live_hotswap(L, settings.mainfile);
}


void lutro_live_draw()
{

}
