#include "live.h"
#include "lutro.h"
#include "file/file_path.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/inotify.h>
#include <unistd.h>

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

//   lua_pushnumber(L, RETRO_DEVICE_ID_JOYPAD_UP);
//   lua_setfield(L, -2, "JOY_UP");
//   lua_pushnumber(L, RETRO_DEVICE_ID_JOYPAD_DOWN);
//   lua_setfield(L, -2, "JOY_DOWN");
//   lua_pushnumber(L, RETRO_DEVICE_ID_JOYPAD_LEFT);
//   lua_setfield(L, -2, "JOY_LEFT");
//   lua_pushnumber(L, RETRO_DEVICE_ID_JOYPAD_RIGHT);
//   lua_setfield(L, -2, "JOY_RIGHT");
//   lua_pushnumber(L, RETRO_DEVICE_ID_JOYPAD_A);
//   lua_setfield(L, -2, "JOY_A");
//   lua_pushnumber(L, RETRO_DEVICE_ID_JOYPAD_B);
//   lua_setfield(L, -2, "JOY_B");
//   lua_pushnumber(L, RETRO_DEVICE_ID_JOYPAD_START);
//   lua_setfield(L, -2, "JOY_START");

   lua_setfield(L, -2, "live");

   return 1;
}

void lutro_live_init()
{
   char mainpath[PATH_MAX_LENGTH] = {0};
   fill_pathname_join(mainpath, settings.gamedir, "main.lua", PATH_MAX_LENGTH);

   memset(&live, 0, sizeof(live));

   live.ifd = inotify_init1(IN_NONBLOCK);

   if (live.ifd == -1) {
      fprintf(stderr, "disabling lutro.live due to inotify initialization failure: %s", strerror(errno));
      settings.live = 0;
      return;
   }

   live.mfd = inotify_add_watch(live.ifd, mainpath, IN_MODIFY);
   if (!live.mfd) {
      fprintf(stderr, "lutro.live failed to monitor main.lua: %s", strerror(errno));
      settings.live = 0;
      lutro_live_deinit();
      return;
   }
}

void lutro_live_deinit()
{
   if (live.mfd >= 0)
      close(live.mfd);

   if (live.ifd >= 0)
      close(live.ifd);
}
