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

# system platform
system_platform = unix
ifeq ($(shell uname -a),)
	EXE_EXT = .exe
	system_platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
	system_platform = osx
	arch = intel
ifeq ($(shell uname -p),powerpc)
	arch = ppc
endif
else ifneq ($(findstring MINGW,$(shell uname -a)),)
	system_platform = win
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

ifeq ($(ARCH), $(filter $(ARCH), intel))
	WANT_JIT = 1
endif
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

ifeq ($(ARCH), $(filter $(ARCH), intel))
	WANT_JIT = 1
endif

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
else ifeq ($(platform), ngc)
	TARGET := $(TARGET_NAME)_libretro_ngc.a
	CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	CC_AS = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	CXX = $(DEVKITPPC)/bin/powerpc-eabi-g++$(EXE_EXT)
	AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
	DEFINES += -DGEKKO -DHW_DOL -mrvl -mcpu=750 -meabi -mhard-float
   LUA_MYCFLAGS := $(DEFINES) $(CFLAGS)
	STATIC_LINKING = 1
else ifeq ($(platform), wii)
	TARGET := $(TARGET_NAME)_libretro_wii.a
	CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	CC_AS = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	CXX = $(DEVKITPPC)/bin/powerpc-eabi-g++$(EXE_EXT)
	AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
	DEFINES += -DGEKKO -DHW_RVL -mrvl -mcpu=750 -meabi -mhard-float
   LUA_MYCFLAGS := $(DEFINES) $(CFLAGS)
	STATIC_LINKING = 1
# PS3
else ifeq ($(platform), ps3)
	TARGET := $(TARGET_NAME)_libretro_ps3.a
	CC = $(CELL_SDK)/host-win32/ppu/bin/ppu-lv2-gcc.exe
	CC_AS = $(CELL_SDK)/host-win32/ppu/bin/ppu-lv2-gcc.exe
	CXX = $(CELL_SDK)/host-win32/ppu/bin/ppu-lv2-g++.exe
	AR = $(CELL_SDK)/host-win32/ppu/bin/ppu-lv2-ar.exe
	DEFINES := -D__CELLOS_LV2__
   LUA_MYCFLAGS := $(DEFINES) $(CFLAGS)
	STATIC_LINKING = 1

# sncps3
else ifeq ($(platform), sncps3)
	TARGET := $(TARGET_NAME)_libretro_ps3.a
	CC = $(CELL_SDK)/host-win32/sn/bin/ps3ppusnc.exe
	CC_AS = $(CELL_SDK)/host-win32/sn/bin/ps3ppusnc.exe
	CXX = $(CELL_SDK)/host-win32/sn/bin/ps3ppusnc.exe
	AR = $(CELL_SDK)/host-win32/sn/bin/ps3snarl.exe
	DEFINES := -D__CELLOS_LV2__
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
LUALIB := $(LUADIR)/liblua.a
ifeq ($(WANT_JIT),1)
   LUADIR := deps/luajit/src
   LUALIB := $(LUADIR)/libluajit.a
   LIBS += -ldl
   CFLAGS += -DHAVE_JIT
endif

CFLAGS += -I$(LUADIR)

LIBS += $(LUALIB) $(LIBM)

ifeq ($(platform), qnx)
   CFLAGS += -Wc,-std=gnu99
else ifneq ($(platform), sncps3)
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
	$(MAKE) -C deps/lua/src CC="$(CC)" CXX="$(CXX)" MYCFLAGS="$(LUA_MYCFLAGS) -w -g $(fpic)" MYLDFLAGS="$(LFLAGS) $(fpic)" SYSCFLAGS="$(LUA_SYSCFLAGS) $(fpic)" a

deps/luajit/src/libluajit.a:
	$(MAKE) -C deps/luajit/src BUILDMODE=static CFLAGS="$(LUA_MYCFLAGS) $(fpic)" Q= LDFLAGS="$(fpic)"

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-make -C $(LUADIR) clean
	-rm -f $(OBJS) $(TARGET)

.PHONY: clean
