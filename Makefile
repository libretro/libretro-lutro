
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
else ifeq ($(platform), linux-portable)
   TARGET := $(TARGET_NAME)_libretro.so
   fpic := -fPIC -nostdlib
   SHARED := -shared -Wl,--no-undefined
   LUA_SYSCFLAGS := -DLUA_USE_POSIX
	LIBM :=
	LDFLAGS += -L. -lmusl
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

LDFLAGS += $(LIBM)

OBJECTS := libretro.o lutro.o runtime.o live.o \
           graphics.o input.o audio.o filesystem.o timer.o \
           libretro-common/formats/png/rpng_decode_fbio.o \
           libretro-common/file/file_path.o \
           libretro-common/compat/compat.o \
           ioapi.o \
           unzip.o

CFLAGS += -Wall -pedantic $(fpic) -I./libretro-common/include

LFLAGS := $(shell pkg-config --libs-only-L --libs-only-other $(packages)) -Wl,-E
LIBS := lua/src/liblua.a $(shell pkg-config --libs-only-l $(packages)) $(LDFLAGS) -lz

ifeq ($(platform), qnx)
   CFLAGS += -Wc,-std=gnu99
else
   CFLAGS += -std=gnu99
endif

all: $(TARGET)

$(TARGET): $(OBJECTS) lua/src/liblua.a
	$(CC) $(fpic) $(SHARED) $(INCLUDES) $(LFLAGS) -o $@ $(OBJECTS) $(LIBS)

lua/src/liblua.a:
	$(MAKE) -C lua/src/ CC=$(CC) CXX=$(CXX) MYCFLAGS="$(LUA_MYCFLAGS) -w -g" MYLDFLAGS="$(LFLAGS)" SYSCFLAGS="$(LUA_SYSCFLAGS) $(fpic)" a

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: clean
