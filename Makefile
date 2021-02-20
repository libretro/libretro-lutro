HAVE_INOTIFY=0
HAVE_COMPOSITION=0
WANT_JIT=0
WANT_ZLIB=1
WANT_UNZIP=1
WANT_LUASOCKET=0
WANT_PHYSFS=1

MMD := -MMD

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
ifeq ($(shell uname -p),arm)
	arch = arm
endif
ifeq ($(shell uname -p),powerpc)
	arch = ppc
endif
else ifneq ($(findstring MINGW,$(shell uname -a)),)
	system_platform = win
endif

TARGET_NAME := lutro
GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
	GVFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif
LIBM := -lm
STATIC_LINKING := 0


ifeq ($(platform), unix)
   TARGET := $(TARGET_NAME)_libretro.so
   fpic := -fPIC
   SHARED := -shared -Wl,--no-as-needed,--no-undefined
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
   WANT_PHYSFS=0
   MMD :=

ifeq ($(UNIVERSAL),1)
ifeq ($(ARCHFLAGS),)
   ARCHFLAGS = -arch i386 -arch x86_64
endif
ifeq ($(archs),arm)
   ARCHFLAGS = -arch arm64
endif
ifeq ($(archs),ppc)
   ARCHFLAGS = -arch ppc -arch ppc64
endif
endif

ifeq ($(CROSS_COMPILE),1)
	TARGET_RULE   = -target $(LIBRETRO_APPLE_PLATFORM) -isysroot $(LIBRETRO_APPLE_ISYSROOT)
	CC       += $(TARGET_RULE)
	CFLAGS   += $(TARGET_RULE)
	CPPFLAGS += $(TARGET_RULE)
	CXXFLAGS += $(TARGET_RULE)
	LDFLAGS  += $(TARGET_RULE)
	LUA_SYSCFLAGS += $(TARGET_RULE)
	CFLAGS += -DDONT_WANT_ARM_OPTIMIZATIONS
else
ifeq ($(shell uname -p),arm)
   CFLAGS += -DDONT_WANT_ARM_OPTIMIZATIONS
endif
endif

	CFLAGS  += $(ARCHFLAGS)
	CXXFLAGS  += $(ARCHFLAGS)
	LDFLAGS += $(ARCHFLAGS)

# iOS
else ifneq (,$(findstring ios,$(platform)))

   TARGET := $(TARGET_NAME)_libretro_ios.dylib
   fpic := -fPIC
   SHARED := -dynamiclib
   DEFINES := -DIOS
   CFLAGS += -DHAVE_STRL
ifeq ($(IOSSDK),)
   IOSSDK := $(shell xcodebuild -version -sdk iphoneos Path)
endif
ifeq ($(platform),ios-arm64)
  CC = cc -arch arm64 -isysroot $(IOSSDK)
else
  CC = cc -arch armv7 -isysroot $(IOSSDK)
endif
IPHONEMINVER :=
ifeq ($(platform),ios9)
   IPHONEMINVER = -miphoneos-version-min=8.0
else
   IPHONEMINVER = -miphoneos-version-min=6.0
endif
   LDFLAGS += $(IPHONEMINVER)
   FLAGS += $(IPHONEMINVER)
   CCFLAGS += $(IPHONEMINVER)
   CXXFLAGS += $(IPHONEMINVER)
   WANT_PHYSFS=0

else ifeq ($(platform), qnx)
   TARGET := $(TARGET_NAME)_libretro_$(platform).so
   fpic := -fPIC
   SHARED := -shared -Wl,--no-undefined
   MMD :=
   CC = qcc -Vgcc_ntoarmv7le
   CXX = QCC -Vgcc_ntoarmv7le
   # Blackberry is fully capable to do thumb2 but gcc 4.6.3 crashes on
   # physfs/7z compilation. Also cdrom part of physfs fails to compile
   # but we don't need it anyway
   CFLAGS += -marm -mthumb-interwork -DPHYSFS_NO_CDROM_SUPPORT=1
else ifeq ($(platform), emscripten)
   TARGET := $(TARGET_NAME)_libretro_$(platform).bc
   STATIC_LINKING = 1

# PSP
else ifeq ($(platform), psp1)
   TARGET := $(TARGET_NAME)_libretro_$(platform).a
   fpic :=
   CC = psp-gcc$(EXE_EXT)
   CXX = psp-g++$(EXE_EXT)
   AR = psp-ar$(EXE_EXT)
   DEFINES := -DPSP -G0 -DLSB_FIRST -DHAVE_ASPRINTF
   CFLAGS += -march=allegrex -mfp32 -mgp32 -mlong32 -mabi=eabi
   CFLAGS += -fomit-frame-pointer -fstrict-aliasing
   CFLAGS += -falign-functions=32 -falign-loops -falign-labels -falign-jumps
   CFLAGS += -I$(shell psp-config --pspsdk-path)/include
   LDFLAGS += $(DEVKITPSP)psp/lib/libc.a $(DEVKITPSP)psp/sdk/lib/libpspkernel.a
   LUA_MYCFLAGS := $(DEFINES) $(CFLAGS)
   STATIC_LINKING = 1
	WANT_PHYSFS=0
   MMD :=

# Vita
else ifeq ($(platform), vita)
   TARGET := $(TARGET_NAME)_libretro_$(platform).a
   fpic := -fno-PIC
	CC = arm-vita-eabi-gcc$(EXE_EXT)
	CXX = arm-vita-eabi-g++$(EXE_EXT)
	AR = arm-vita-eabi-ar$(EXE_EXT)
   DEFINES := -DVITA  -DHAVE_ASPRINTF
   LUA_MYCFLAGS := $(DEFINES) $(CFLAGS)
   STATIC_LINKING = 1
	WANT_PHYSFS=0
   MMD :=

else ifeq ($(platform), ngc)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	CC_AS = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	CXX = $(DEVKITPPC)/bin/powerpc-eabi-g++$(EXE_EXT)
	AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
	DEFINES += -DGEKKO -DHW_DOL -mrvl -mcpu=750 -meabi -mhard-float
	WANT_PHYSFS=0
   LUA_MYCFLAGS := $(DEFINES) $(CFLAGS)
	STATIC_LINKING = 1
   MMD :=

else ifeq ($(platform), wii)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	CC_AS = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	CXX = $(DEVKITPPC)/bin/powerpc-eabi-g++$(EXE_EXT)
	AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
	DEFINES += -DGEKKO -DHW_RVL -mrvl -mcpu=750 -meabi -mhard-float
	WANT_PHYSFS=0
   LUA_MYCFLAGS := $(DEFINES) $(CFLAGS)
	STATIC_LINKING = 1
   MMD :=

else ifeq ($(platform), wiiu)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	CC_AS = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	CXX = $(DEVKITPPC)/bin/powerpc-eabi-g++$(EXE_EXT)
	AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
	DEFINES += -DGEKKO -DWIIU -DHW_RVL -mrvl -mcpu=750 -meabi -mhard-float
   LUA_MYCFLAGS := $(DEFINES) $(CFLAGS)
	STATIC_LINKING = 1
	WANT_PHYSFS=0
   MMD :=

# CTR(3DS)
else ifeq ($(platform), ctr)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = $(DEVKITARM)/bin/arm-none-eabi-gcc$(EXE_EXT)
	CXX = $(DEVKITARM)/bin/arm-none-eabi-g++$(EXE_EXT)
	AR = $(DEVKITARM)/bin/arm-none-eabi-ar$(EXE_EXT)
	DEFINES += -DARM11 -D_3DS -march=armv6k -mtune=mpcore -mfloat-abi=hard
	LUA_MYCFLAGS := $(DEFINES) $(CFLAGS)
	STATIC_LINKING :=1
	WANT_PHYSFS=0
	MMD :=
	DEFINES += -DDONT_WANT_ARM_OPTIMIZATIONS

# Nintendo Switch (libnx)
else ifeq ($(platform), libnx)
	include $(DEVKITPRO)/libnx/switch_rules
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	fpic := -fPIC
	DEFINES += -D__SWITCH__ -DHAVE_LIBNX -I$(LIBNX)/include/ -specs=$(LIBNX)/switch.specs
	DEFINES += -march=armv8-a -mtune=cortex-a57 -mtp=soft -mcpu=cortex-a57+crc+fp+simd -ffast-math
	LUA_MYCFLAGS := $(DEFINES) $(CFLAGS)
	STATIC_LINKING=1
	WANT_PHYSFS=0
	MMD :=

# PS3
else ifeq ($(platform), ps3)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = $(CELL_SDK)/host-win32/ppu/bin/ppu-lv2-gcc.exe
	CC_AS = $(CELL_SDK)/host-win32/ppu/bin/ppu-lv2-gcc.exe
	CXX = $(CELL_SDK)/host-win32/ppu/bin/ppu-lv2-g++.exe
	AR = $(CELL_SDK)/host-win32/ppu/bin/ppu-lv2-ar.exe
	DEFINES := -D__CELLOS_LV2__
   LUA_MYCFLAGS := $(DEFINES) $(CFLAGS)
	STATIC_LINKING = 1
   MMD :=

# ARM
else ifneq (,$(findstring armv,$(platform)))
   TARGET := $(TARGET_NAME)_libretro.so
   SHARED := -shared -Wl,--no-undefined
   fpic := -fPIC
   CFLAGS += -D_GNU_SOURCE=1
ifneq (,$(findstring cortexa8,$(platform)))
   CFLAGS += -marm -mcpu=cortex-a8
   ASFLAGS += -mcpu=cortex-a8
else ifneq (,$(findstring cortexa9,$(platform)))
   CFLAGS += -marm -mcpu=cortex-a9
   ASFLAGS += -mcpu=cortex-a9
endif
   CFLAGS += -marm
ifneq (,$(findstring neon,$(platform)))
   CFLAGS += -mfpu=neon
   ASFLAGS += -mfpu=neon
   HAVE_NEON = 1
endif
ifneq (,$(findstring softfloat,$(platform)))
   CFLAGS += -mfloat-abi=softfp
   ASFLAGS += -mfloat-abi=softfp
else ifneq (,$(findstring hardfloat,$(platform)))
   CFLAGS += -mfloat-abi=hard
   ASFLAGS += -mfloat-abi=hard
endif
   CFLAGS += -DARM

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
   MMD :=
else
   CC ?= gcc
   TARGET := $(TARGET_NAME)_libretro.dll
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

OBJS += $(SOURCES_C:.c=.o) $(SOURCES_CXX:.cpp=.o) $(SOURCES_ASM:.S=.o)

CFLAGS += -Wall -pedantic $(fpic) $(INCFLAGS)

LUADIR := deps/lua/src
LUALIB := $(LUADIR)/liblua.a
ifeq ($(WANT_JIT),1)
   LUADIR := deps/luajit/src
   LUALIB := $(LUADIR)/libluajit.a
   ifeq ($(platform), unix)
      LIBS += -ldl
   endif
   CFLAGS += -DHAVE_JIT
endif

CFLAGS += -I$(LUADIR) $(DEFINES) -DOUTSIDE_SPEEX -DRANDOM_PREFIX=speex -DEXPORT= -DFIXED_POINT

LIBS += $(LUALIB) $(LIBM)

ifeq ($(platform), qnx)
   CFLAGS += -Wc,-std=gnu99
else ifneq ($(platform), sncps3)
   CFLAGS += -std=gnu99
endif


ifneq ($(SANITIZER),)
   CFLAGS += -fsanitize=$(SANITIZER)
   LDFLAGS += -fsanitize=$(SANITIZER)
   SHARED := -shared
endif

ifeq ($(platform), osx)
ifndef ($(NOUNIVERSAL))
   CFLAGS += $(ARCHFLAGS)
   LFLAGS += $(ARCHFLAGS)
endif
endif

ifeq ($(platform),ios-arm64)
	LUADEFINES = -DIOS
endif

OBJS := $(addprefix obj/,$(OBJS))

all: $(TARGET)

ifneq ($(MMD),)
-include $(OBJS:.o=.d)
endif

$(TARGET): $(OBJS) $(LUALIB)
ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $(OBJS) $(LUADIR)/*.o
else
	$(CC) $(fpic) $(SHARED) $(INCLUDES) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
endif

deps/lua/src/liblua.a:
	$(MAKE) -C deps/lua/src CC="$(CC) $(LUADEFINES)" CXX="$(CXX)" MYCFLAGS="$(LUA_MYCFLAGS) -w $(fpic)" MYLDFLAGS="$(LDFLAGS) $(fpic)" SYSCFLAGS="$(LUA_SYSCFLAGS) $(fpic)" liblua.a
deps/luajit/src/libluajit.a:
	$(MAKE) -C deps/luajit/src CC="$(CC)" CXX="$(CXX)" HOST_CC="$(HOST_CC) $(PTR_SIZE)" CROSS="$(CROSS)" BUILDMODE=static CFLAGS="$(LUA_MYCFLAGS) -w $(fpic)" Q= LDFLAGS="$(LDFLAGS) $(fpic)" libluajit.a

obj/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(GVFLAGS) $(MMD) -c -o $@ $<

clean:
	-make -C $(LUADIR) clean
	-rm -f $(OBJS) $(TARGET)
	-rm -rf obj

test: all
	retroarch --verbose -L lutro_libretro.so test/main.lua

.PHONY: clean
