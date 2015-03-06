HAVE_INOTIFY=0
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

ifeq ($(platform), unix)
   TARGET := $(TARGET_NAME)_libretro.so
   fpic := -fPIC
   SHARED := -shared -Wl,--no-undefined
   LUA_SYSCFLAGS := -DLUA_USE_POSIX
   HAVE_INOTIFY=1
else ifeq ($(platform), linux-portable)
   TARGET := $(TARGET_NAME)_libretro.so
   fpic := -fPIC -nostdlib
   SHARED := -shared
   HAVE_INOTIFY=1
   LUA_SYSCFLAGS := -DLUA_USE_POSIX
   LIBM :=
else ifeq ($(platform), osx)
   TARGET := $(TARGET_NAME)_libretro.dylib
   fpic := -fPIC
   SHARED := -dynamiclib
   LUA_SYSCFLAGS := -DLUA_USE_MACOSX
else ifeq ($(platform), ios)
   TARGET := $(TARGET_NAME)_libretro_ios.dylib
   fpic := -fPIC
   SHARED := -dynamiclib
   DEFINES := -DIOS
   CC = clang -arch armv7 -isysroot $(IOSSDK)
else ifeq ($(platform), qnx)
   TARGET := $(TARGET_NAME)_libretro_qnx.so
   fpic := -fPIC
   SHARED := -shared -Wl,--no-undefined
else ifeq ($(platform), emscripten)
   TARGET := $(TARGET_NAME)_libretro_emscripten.so
   fpic := -fPIC
   SHARED := -shared -Wl,--no-undefined
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

LDFLAGS += $(LIBM)

include Makefile.common

OBJS += $(SOURCES_C:.c=.o) $(SOURCES_CXX:.cpp=.o)

CFLAGS += -Wall -pedantic $(fpic) $(INCFLAGS)

LFLAGS := $(shell pkg-config --libs-only-L --libs-only-other $(packages)) -Wl,-E
LIBS := deps/lua/src/liblua.a $(shell pkg-config --libs-only-l $(packages)) $(LDFLAGS)

ifeq ($(platform), qnx)
   CFLAGS += -Wc,-std=gnu99
else
   CFLAGS += -std=gnu99
endif

all: $(TARGET)

$(TARGET): $(OBJS) deps/lua/src/liblua.a
	$(CC) $(fpic) $(SHARED) $(INCLUDES) $(LFLAGS) -o $@ $(OBJS) $(LIBS)

deps/lua/src/liblua.a:
	$(MAKE) -C deps/lua/src/ CC=$(CC) CXX=$(CXX) MYCFLAGS="$(LUA_MYCFLAGS) -w -g" MYLDFLAGS="$(LFLAGS)" SYSCFLAGS="$(LUA_SYSCFLAGS) $(fpic)" a

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-make clean -C deps/lua/src clean
	-rm -f $(OBJS) $(TARGET)

.PHONY: clean
