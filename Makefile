HAVE_INOTIFY=0
HAVE_COMPOSITION=0
WANT_JIT=0
WANT_ZLIB=1

ifneq ($(EMSCRIPTEN),)
   platform = emscripten
endif

ifeq ($(platform),)
platform = unix
ifeq ($(shell uname -a),)
   platform = win
else ifneq ($(findstring MINGW,$(shell uname -a)),)
   platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
   platform = osx
else ifneq ($(findstring win,$(shell uname -a)),)
   platform = win
endif
endif

TARGET_NAME := lutro
LUA_MYCFLAGS :=
LUA_SYSCFLAGS :=
LIBM := -lm
STATIC_LINKING := 0

ifeq ($(platform), unix)
   TARGET := $(TARGET_NAME)_libretro.so
   fpic := -fPIC
   SHARED := -shared -Wl,--no-undefined
   LUA_SYSCFLAGS := -DLUA_USE_POSIX
   HAVE_INOTIFY=1
   LDFLAGS += -Wl,-E
else ifeq ($(platform), linux-portable)
   TARGET := $(TARGET_NAME)_libretro.so
   fpic := -fPIC -nostdlib
   SHARED := -shared
   HAVE_INOTIFY=1
   LUA_SYSCFLAGS := -DLUA_USE_POSIX
   LIBM :=
   LDFLAGS += -Wl,-E
else ifeq ($(platform), osx)
   TARGET := $(TARGET_NAME)_libretro.dylib
   fpic := -fPIC
   SHARED := -dynamiclib
   LUA_SYSCFLAGS := -DLUA_USE_MACOSX
   CFLAGS += -DHAVE_STRL
   # for 64bit osx:
   #ifeq ($(WANT_JIT))
   #   LDFLAGS += -Wl,-pagezero_size,10000 -Wl,-image_base,100000000
   #endif
else ifeq ($(platform), ios)
   TARGET := $(TARGET_NAME)_libretro_ios.dylib
   fpic := -fPIC
   SHARED := -dynamiclib
   DEFINES := -DIOS
   CC = clang -arch armv7 -isysroot $(IOSSDK)
   CFLAGS += -DHAVE_STRL
else ifeq ($(platform), qnx)
   TARGET := $(TARGET_NAME)_libretro_qnx.so
   fpic := -fPIC
   SHARED := -shared -Wl,--no-undefined
else ifeq ($(platform), emscripten)
   TARGET := $(TARGET_NAME)_libretro_emscripten.so
   fpic := -fPIC
   SHARED := -shared -Wl,--no-undefined
else ifeq ($(platform), psp1)
   TARGET := $(TARGET_NAME)_libretro_psp1.a
   fpic :=
   CC = psp-gcc$(EXE_EXT)
   CXX = psp-g++$(EXE_EXT)
   AR = psp-ar$(EXE_EXT)
   DEFINES := -DPSP -G0 -DLSB_FIRST -DHAVE_ASPRINTF
   CFLAGS += -march=allegrex -mfp32 -mgp32 -mlong32 -mabi=eabi
   CFLAGS += -fomit-frame-pointer -fstrict-aliasing
   CFLAGS += -falign-functions=32 -falign-loops -falign-labels -falign-jumps
   CFLAGS += -I$(shell psp-config --pspsdk-path)/include
   LUA_MYCFLAGS := $(DEFINES) $(CFLAGS)
   STATIC_LINKING = 1
else
   CC = gcc
   TARGET := $(TARGET_NAME)_retro.dll
   SHARED := -shared -static-libgcc -static-libstdc++ -s -Wl,--no-undefined
endif

ifeq ($(DEBUG), 1)
   CFLAGS += -O0 -g
   LUA_MYCFLAGS += -O0 -g -DLUA_USE_APICHECK
else
   CFLAGS += -O3
   LUA_MYCFLAGS += -O3
endif

CORE_DIR := .

include Makefile.common

OBJS += $(SOURCES_C:.c=.o) $(SOURCES_CXX:.cpp=.o)

CFLAGS += -Wall -pedantic $(fpic) $(INCFLAGS)

LUADIR := deps/lua/src
LUALIB := $(LUALIB)/liblua.a
ifeq ($(WANT_JIT),1)
   LUADIR := deps/luajit/src
   LUALIB := $(LUADIR)/libluajit.a
   LIBS += -ldl
endif

CFLAGS += -I$(LUADIR)

LIBS += $(LUALIB) $(LIBM)

ifeq ($(platform), qnx)
   CFLAGS += -Wc,-std=gnu99
else
   CFLAGS += -std=gnu99
endif


all: $(TARGET)

$(TARGET): $(OBJS) $(LUALIB)
ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $(OBJS) $(LUADIR)/*.o
else
	$(CC) $(fpic) $(SHARED) $(INCLUDES) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
endif

deps/lua/src/liblua.a:
	$(MAKE) -C deps/lua/src CC="$(CC)" CXX="$(CXX)" MYCFLAGS="$(LUA_MYCFLAGS) -w -g" MYLDFLAGS="$(LFLAGS)" SYSCFLAGS="$(LUA_SYSCFLAGS) $(fpic)" a

deps/luajit/src/libluajit.a:
	$(MAKE) -C deps/luajit/src BUILDMODE=static CFLAGS="$(LUA_MYCFLAGS) $(fpic)" Q= LDFLAGS="$(fpic)"

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-make -C $(LUADIR) clean
	-rm -f $(OBJS) $(TARGET)

.PHONY: clean
