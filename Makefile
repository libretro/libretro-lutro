
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

ifeq ($(platform), unix)
   TARGET := $(TARGET_NAME)_libretro.so
   fpic := -fPIC
   SHARED := -shared -Wl,--no-undefined
else ifeq ($(platform), osx)
   TARGET := $(TARGET_NAME)_libretro.dylib
   fpic := -fPIC
   SHARED := -dynamiclib
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
   CFLAGS += -O0 -g -DLUA_USE_APICHECK
else
   CFLAGS += -O3
endif

OBJECTS := libretro.o lutro.o runtime.o live.o \
           graphics.o input.o audio.o filesystem.o \
           libretro-sdk/formats/png/rpng.o \
           libretro-sdk/file/file_path.o \
           libretro-sdk/compat/compat.o \
           ioapi.o \
           unzip.o

CFLAGS += -Wall -pedantic $(fpic) -I./libretro-sdk/include

LFLAGS := $(shell pkg-config --libs-only-L --libs-only-other $(packages)) -Wl,-E
LIBS := lua/src/liblua.a $(shell pkg-config --libs-only-l $(packages)) -lm -lz

ifeq ($(platform), qnx)
   CFLAGS += -Wc,-std=gnu99
else
   CFLAGS += -std=gnu99
endif

all: $(TARGET)

$(TARGET): $(OBJECTS) lua/src/liblua.a
	$(CC) $(fpic) $(SHARED) $(INCLUDES) $(LFLAGS) -o $@ $(OBJECTS) $(LIBS)

lua/src/liblua.a:
	$(MAKE) -C lua/src/ CC=$(CC) CXX=$(CXX) MYCFLAGS="$(CFLAGS) -w -g" MYLDFLAGS="$(LFLAGS)" a

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: clean
