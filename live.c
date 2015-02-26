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
   int mfd; // main fd
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
      fprintf(stderr, "disabling lutro.live due to inotify initialization failure: %s\n", strerror(errno));
      lutro_live_deinit();
      return;
   }

   // XXX: Some editors do not trigger IN_MODIFY since they write to a temp file
   //      and rename() it to the actual file.
   live.mfd = inotify_add_watch(live.ifd, settings.mainfile, IN_MODIFY);
   if (live.mfd < 0)
   {
      fprintf(stderr, "lutro.live failed to monitor main.lua: %s\n", strerror(errno));
      lutro_live_deinit();
      return;
   }
}

void lutro_live_deinit()
{
   settings.live_enable = 0;

   if (live.mfd >= 0)
      close(live.mfd);

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

// TODO: check if there's anything else that needs to be backed up
static void live_swap(lua_State *L)
{
   int backup = 0;

   lutro_checked_stack_begin();

   // backup _G then drop it
   lua_pushglobaltable(L);
   lua_newtable(L);
   shallow_update(L, -2, -1);
   lua_remove(L, -2);

   backup = lua_absindex(L, -1);

   if(luaL_dofile(L, settings.mainfile))
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
      fprintf(stderr, "innotify read error: %s\n", strerror(errno));
      return;
   }

   if (modified)
      live_swap(L);
}


void lutro_live_draw()
{

}
