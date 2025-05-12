
#### CLI OPTIONS

# build configurations.
#   config=debug   full debug of player and tool portions of the engine (-O0)
#                  mainly for use in-house by Lutro engine developers.
#   config=tool    provides logging, assertion checking, special LUA APIs for content creators.
#                  this is the default config and is how releases of Lutro should be shipped to Love2D devs.
#   config=player  most assertions disabled or demoted to logs, special tooling APIs disabled.
#                  meant for packaging and shipping a Love2D game as a standalone playable 'finished product'.
#
# example:
#   $ make config=[debug,tool,player])

# copy config -> LUTRO_CONFIG. 'config' is just a shorthand that we provide for the command line.
# it's better to keep these vars in slightly strong named vars for use through-out Makefile tho.

LUTRO_CONFIG ?= $(config)

# WANTS use ?= syntax to allow things to be provided via environment as well as via CLI
# NOTE: if you change these on the CLI then you MUST manually run clean!
# (this could be fixed with some clever make scripting, maybe later...)

WANT_JIT         ?= 0
WANT_ZLIB        ?= 1
WANT_UNZIP       ?= 1
WANT_LUASOCKET   ?= 0
WANT_PHYSFS      ?= 0
WANT_COMPOSITION ?= 1
WANT_TRANSFORM   ?= 1
TRACE_ALLOCATION ?= 0

#### END CLI OPTIONS

# setup some things that will be reassigned per-platform
HAVE_INOTIFY ?= 0
MMD := -MMD

ifeq ($(LUTRO_CONFIG),)
    LUTRO_CONFIG = player
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
ifndef GIT_VERSION
GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
endif
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
    LDFLAGS += -Wl,-E
else ifeq ($(platform), linux-portable)
    TARGET := $(TARGET_NAME)_libretro.so
    fpic := -fPIC -nostdlib
    SHARED := -shared
    LUA_SYSCFLAGS := -DLUA_USE_POSIX
    LIBM :=
    LDFLAGS += -Wl,-E
else ifeq ($(platform), osx)
    TARGET := $(TARGET_NAME)_libretro.dylib
    fpic := -fPIC
    SHARED := -dynamiclib
    LUA_SYSCFLAGS := -DLUA_USE_MACOSX
    CFLAGS += -DHAVE_STRL -DDONT_WANT_ARM_OPTIMIZATIONS
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
    CFLAGS += -DHAVE_STRL -DDONT_WANT_ARM_OPTIMIZATIONS
    MINVERSION   :=
    ifeq ($(IOSSDK),)
        IOSSDK := $(shell xcodebuild -version -sdk iphoneos Path)
    endif
ifeq ($(platform),$(filter $(platform),ios9 ios-arm64))
    CC = cc -arch arm64 -isysroot $(IOSSDK)
else
    CC = cc -arch armv7 -isysroot $(IOSSDK)
endif
    ifeq ($(platform),ios9)
        MINVERSION  = -miphoneos-version-min=8.0
    else
        MINVERSION = -miphoneos-version-min=6.0
    endif
    LDFLAGS      += $(MINVERSION)
    FLAGS        += $(MINVERSION)
    CFLAGS       += $(MINVERSION)
    CXXFLAGS     += $(MINVERSION)
    LUA_MYCFLAGS := $(MINVERSION)
    WANT_PHYSFS=0
    LUADEFINES = -DIOS

else ifeq ($(platform), tvos-arm64)
    TARGET := $(TARGET_NAME)_libretro_tvos.dylib
    fpic := -fPIC
    SHARED := -dynamiclib
    DEFINES := -DIOS
    CFLAGS += -DHAVE_STRL -DDONT_WANT_ARM_OPTIMIZATIONS
    MINVERSION   := -mappletvos-version-min=11.0
    ifeq ($(IOSSDK),)
        IOSSDK := $(shell xcodebuild -version -sdk appletvos Path)
    endif
    CC = cc -arch arm64 -isysroot $(IOSSDK)
    LDFLAGS      += $(MINVERSION)
    FLAGS        += $(MINVERSION)
    CFLAGS       += $(MINVERSION)
    CXXFLAGS     += $(MINVERSION)
    LUA_MYCFLAGS := $(MINVERSION)
    WANT_PHYSFS=0
    LUADEFINES = -DIOS

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

# PS2
else ifeq ($(platform),ps2)
    TARGET := $(TARGET_NAME)_libretro_$(platform).a
    CC = mips64r5900el-ps2-elf-gcc$(EXE_EXT)
    CXX = mips64r5900el-ps2-elf-g++$(EXE_EXT)
    AR = mips64r5900el-ps2-elf-ar$(EXE_EXT)
    TARGET := $(TARGET_NAME)_libretro_$(platform).a
    fpic := -fno-PIC
    DEFINES := -G0 -DPS2 -DABGR -DHAVE_NO_LANGEXTRA -O3
    LUA_MYCFLAGS := $(DEFINES) $(CFLAGS)
    STATIC_LINKING = 1
    WANT_PHYSFS=0

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

# PS4    
else ifeq ($(platform), ps4)
    TARGET := $(TARGET_NAME)_libretro_$(platform).a
    DEFINES := -D__ORBIS__ -D__PS4__
    CFLAGS += $(DEFINES) -O2 -std=gnu11 -fPIC -funwind-tables
    CXXFLAGS += $(DEFINES) -O2 -std=gnu++11
    LDFLAGS += -Wl,--gc-sections -m elf_x86_64 -pie
    STATIC_LINKING = 0
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
# RS90
else ifeq ($(platform), rs90)
    TARGET := $(TARGET_NAME)_libretro.so
    CC = /opt/rs90-toolchain/usr/bin/mipsel-linux-gcc
    CXX = /opt/rs90-toolchain/usr/bin/mipsel-linux-g++
    AR = /opt/rs90-toolchain/usr/bin/mipsel-linux-ar
    fpic := -fPIC
    SHARED := -shared -Wl,-version-script=link.T
    PLATFORM_DEFINES := -DCC_RESAMPLER -DCC_RESAMPLER_NO_HIGHPASS
    CFLAGS += -fomit-frame-pointer -ffast-math -march=mips32 -mtune=mips32
    LUA_MYCFLAGS += -fomit-frame-pointer -ffast-math -march=mips32 -mtune=mips32
    CXXFLAGS += $(CFLAGS)

# GCW0
else ifeq ($(platform), gcw0)
    TARGET := $(TARGET_NAME)_libretro.so
    CC = /opt/gcw0-toolchain/usr/bin/mipsel-linux-gcc
    AR = /opt/gcw0-toolchain/usr/bin/mipsel-linux-ar
    fpic := -fPIC
    SHARED := -shared -Wl,--version-script=link.T -Wl,-no-undefined

    DISABLE_ERROR_LOGGING := 1
    CFLAGS += -march=mips32 -mtune=mips32r2 -mhard-float
    LUA_MYCFLAGS += -march=mips32 -mtune=mips32r2 -mhard-float
    LIBS = -lm
# RETROFW
else ifeq ($(platform), retrofw)
    EXT ?= so
    TARGET := $(TARGET_NAME)_libretro.$(EXT)
    CC = /opt/retrofw-toolchain/usr/bin/mipsel-linux-gcc
    AR = /opt/retrofw-toolchain/usr/bin/mipsel-linux-ar
    fpic := -fPIC
    SHARED := -shared -Wl,--version-script=link.T -Wl,--no-undefined
    CFLAGS += -ffast-math -march=mips32 -mtune=mips32 -mhard-float
    LUA_MYCFLAGS += -ffast-math -march=mips32 -mtune=mips32 -mhard-float
    LIBS = -lm
# MIYOO
else ifeq ($(platform), miyoo)
    TARGET := $(TARGET_NAME)_libretro.so
    fpic := -fPIC
    SHARED := -shared -Wl,-version-script=link.T
    CC = /opt/miyoo/usr/bin/arm-linux-gcc
    AR = /opt/miyoo/usr/bin/arm-linux-ar
    PLATFORM_DEFINES += -D_GNU_SOURCE
    CFLAGS += -fomit-frame-pointer -ffast-math -march=armv5te -mtune=arm926ej-s
    CFLAGS += -fno-common -ftree-vectorize -funswitch-loops
    LUA_MYCFLAGS += -fomit-frame-pointer -ffast-math -march=armv5te -mtune=arm926ej-s
    LUA_MYCFLAGS += -fno-common -ftree-vectorize -funswitch-loops
else
    TARGET := $(TARGET_NAME)_libretro.dll
    SHARED := -shared -static-libgcc -static-libstdc++ -s -Wl,--no-undefined
    ifeq ($(WANT_LUASOCKET),1)
        LIBS += -lwsock32 -lws2_32
    endif
endif

compiler_brand := $(shell $(CC) --version | grep -o -m1 clang || echo gcc)
cc_version := $(shell $(CC) -dumpversion)

WARN_FLAGS ?= 

# Clang compiler can pedantically generate compiler errors for printf formatting.
# Printf formatting errors often result in undefined program behavior (crashes).
# This feature is not available on GCC.
ifeq ($(compiler_brand),clang)
    WARN_FLAGS += -Wno-gnu-zero-variadic-macro-arguments
    ifeq ($(shell expr $(cc_version) \> 12), 1)
        WARN_FLAGS += -Werror=format
        WARN_FLAGS += -Werror=format-extra-args
        WARN_FLAGS += -Werror=format-insufficient-args
        WARN_FLAGS += -Werror=format-invalid-specifier
        WARN_FLAGS += -Werror=inconsistent-missing-override
    endif
endif

# platform agnostic defines/flags setup #
# include syms unconditionally. if a platform is size-sensitive or needs syms stripped for other reasons, 
# then it is better to use the strip util (binutils) as a separate step explicitly. A typical process is 
# that a symbolcated version of the binary is stored somewhere and then syms are stripped when the game is 
# packaged for distribution. Bug reports from players can then pull syms from the storage when crashes are
# reported).

CFLAGS += -g
LUA_MYCFLAGS += -g

DEFINES_DEBUG += -DLUTRO_BUILD_IS_PLAYER=0
DEFINES_DEBUG += -DLUTRO_BUILD_IS_TOOL=1
DEFINES_DEBUG += -DLUTRO_ENABLE_ERROR=1
DEFINES_DEBUG += -DLUTRO_ENABLE_ALERT=1
DEFINES_DEBUG += -DLUTRO_ENABLE_ASSERT_TOOL=1
DEFINES_DEBUG += -DLUTRO_ENABLE_ASSERT_DBG=1

DEFINES_TOOL += -DNDEBUG
DEFINES_TOOL += -DLUTRO_BUILD_IS_PLAYER=0
DEFINES_TOOL += -DLUTRO_BUILD_IS_TOOL=1
DEFINES_TOOL += -DLUTRO_ENABLE_ERROR=1
DEFINES_TOOL += -DLUTRO_ENABLE_ALERT=1
DEFINES_TOOL += -DLUTRO_ENABLE_ASSERT_TOOL=1
DEFINES_TOOL += -DLUTRO_ENABLE_ASSERT_DBG=0

DEFINES_PLAYER += -DNDEBUG
DEFINES_PLAYER += -DLUTRO_BUILD_IS_PLAYER=1
DEFINES_PLAYER += -DLUTRO_BUILD_IS_TOOL=0
DEFINES_PLAYER += -DLUTRO_ENABLE_ERROR=1
DEFINES_PLAYER += -DLUTRO_ENABLE_ALERT=0
DEFINES_PLAYER += -DLUTRO_ENABLE_ASSERT_TOOL=0
DEFINES_PLAYER += -DLUTRO_ENABLE_ASSERT_DBG=0

ifeq ($(LUTRO_CONFIG),debug)
    DEFINES += $(DEFINES_DEBUG)
    CFLAGS += -O0 -g
    LUA_MYCFLAGS += -O0 -g -DLUA_USE_APICHECK
else ifeq ($(LUTRO_CONFIG),tool)
    DEFINES += $(DEFINES_TOOL)
    DEFINES += -DNDEBUG
    CFLAGS += -O1 -g
    LUA_MYCFLAGS += -O1 -g -DLUA_USE_APICHECK
else ifeq ($(LUTRO_CONFIG),player)
    DEFINES += $(DEFINES_PLAYER)
    DEFINES += -DNDEBUG
    CFLAGS += -O3 -g
    LUA_MYCFLAGS += -O3 -g
else
    $(info valid config targets are: debug, tool, player)
    $(error invalid or unspecified config target: config=$(LUTRO_CONFIG))
endif

CORE_DIR := .

include Makefile.common

OBJS += $(SOURCES_C:.c=.o) $(VORBIS_SOURCES_C:.c=.o) $(SOURCES_CXX:.cpp=.o) $(SOURCES_ASM:.S=.o)

CFLAGS += -Wall -pedantic $(fpic) $(INCFLAGS) $(WARN_FLAGS)

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

ifeq ($(WANT_LUASOCKET),1)
    CFLAGS += -DHAVE_LUASOCKET
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

INTDIR = obj/$(LUTRO_CONFIG)
OBJS := $(addprefix $(INTDIR)/,$(OBJS))

lutro_sources_file = msbuild/lutro_sources.props

all: $(TARGET)

# TARGET: vcxproj
#
# This target bypasses most config options and generates an msbuild property sheet which is included into
# one or more vcxproj targets. Most build options are still configured via the vcxproj itself (via Visual
# Studio target selector, and other VS IDE things). This just provides a nice way of generating sources and
# include dirs from one authorative list: Makefile.common
#
# Limitations:
#  - msbuild cannot handle easily options per-sourcefile. Such functionality will require multiple msbuild files,
#    or authoring a standalone tool that can can inject the massive amount of red-tape xml boilerplate needed to
#    specify build settings per sourcefile. (the latter is not recommended)
#
#  - if any of the SOURCES_C items are populated using wildcards then this will fail. In that case, there needs
#    to be dependency checks on the contents of each dir (gnu make supports this, just specify dirs as dependencies
#    and any files added/removed/renamed within them will trigger a rule rebuild).
#
.PHONY: vcxproj
vcxproj:  $(lutro_sources_file)

$(lutro_sources_file): FORCE   # Makefile Makefile.common
	@printf "$(lutro_sources_file): Generating with %d sources, %d includes\n" $(words $(SOURCES_C)) $(words $(MSVC_SOURCES_H))
	@>  $(lutro_sources_file) echo   '<?xml version="1.0" encoding="utf-8"?>'
	@>> $(lutro_sources_file) echo   '<!-- AUTO_GENERATED FILE - generated by 'make vcxproj' -->'
	@>> $(lutro_sources_file) echo   '<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">'
	@>> $(lutro_sources_file) echo   '  <ItemGroup>'
	@>> $(lutro_sources_file) printf "    <ClCompile Include=\"../%s\" />\n" $(SOURCES_C) $(SOURCES_CXX)
	@>> $(lutro_sources_file) echo   '  </ItemGroup>'
	@[[ -z "$(MSVC_SOURCES_H)" ]] || \
	 >> $(lutro_sources_file) echo   '  <ItemGroup>' && \
	 >> $(lutro_sources_file) printf "    <ClInclude Include=\"../%s\" />\n" $(MSVC_SOURCES_H) && \
	 >> $(lutro_sources_file) echo   '  </ItemGroup>'
	@>> $(lutro_sources_file) echo   '  <ItemDefinitionGroup>'
	@>> $(lutro_sources_file) echo   '    <ClCompile>'
	@>> $(lutro_sources_file) printf "      <AdditionalIncludeDirectories>%%(AdditionalIncludeDirectories);../%s</AdditionalIncludeDirectories>\n" $(subst -I,,$(filter -I%,$(CFLAGS)))
	@>> $(lutro_sources_file) echo   '    </ClCompile>'
	@>> $(lutro_sources_file) echo   '  </ItemDefinitionGroup>'

	@>> $(lutro_sources_file) printf "  <ItemDefinitionGroup Condition=\"'%s'=='Debug'\">\n" "$$""(Configuration)"
	@>> $(lutro_sources_file) echo   '    <ClCompile>'
	@>> $(lutro_sources_file) printf "      <PreprocessorDefinitions>%%(PreprocessorDefinitions);%s</PreprocessorDefinitions>\n" $(subst -D,,$(filter -D%,$(DEFINES_DEBUG)))
	@>> $(lutro_sources_file) echo   '    </ClCompile>'
	@>> $(lutro_sources_file) echo   '  </ItemDefinitionGroup>'

	@>> $(lutro_sources_file) printf "  <ItemDefinitionGroup Condition=\"'%s'=='Tool'\">\n" "$$""(Configuration)"
	@>> $(lutro_sources_file) echo   '    <ClCompile>'
	@>> $(lutro_sources_file) printf "      <PreprocessorDefinitions>%%(PreprocessorDefinitions);%s</PreprocessorDefinitions>\n" $(subst -D,,$(filter -D%,$(DEFINES_TOOL)))
	@>> $(lutro_sources_file) echo   '    </ClCompile>'
	@>> $(lutro_sources_file) echo   '  </ItemDefinitionGroup>'

	@>> $(lutro_sources_file) printf "  <ItemDefinitionGroup Condition=\"'%s'=='Player'\">\n" "$$""(Configuration)"
	@>> $(lutro_sources_file) echo   '    <ClCompile>'
	@>> $(lutro_sources_file) printf "      <PreprocessorDefinitions>%%(PreprocessorDefinitions);%s</PreprocessorDefinitions>\n" $(subst -D,,$(filter -D%,$(DEFINES_PLAYER)))
	@>> $(lutro_sources_file) echo   '    </ClCompile>'
	@>> $(lutro_sources_file) echo   '  </ItemDefinitionGroup>'

	@>> $(lutro_sources_file) echo   '</Project>'
	@touch msbuild/lutro.vcxproj

ifneq ($(MMD),)
-include $(OBJS:.o=.d)
endif

# hard-link the generared binary into the parent dir. this is forced since its fast and
# the current target may be out-of-date if switching between two existing config builds.
$(TARGET): $(INTDIR)/$(TARGET) FORCE
	ln -f $(INTDIR)/$(TARGET) $(TARGET)

$(INTDIR)/$(TARGET): $(OBJS) $(LUALIB)
ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $(OBJS) $(LUADIR)/*.o
else
	$(CC) $(fpic) $(SHARED) $(INCLUDES) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
endif

deps/lua/src/liblua.a:
	$(MAKE) -C deps/lua/src CC="$(CC) $(LUADEFINES)" CXX="$(CXX)" MYCFLAGS="$(LUA_MYCFLAGS) -w $(fpic)" MYLDFLAGS="$(LDFLAGS) $(fpic)" SYSCFLAGS="$(LUA_SYSCFLAGS) $(fpic)" liblua.a
deps/luajit/src/libluajit.a:
	$(MAKE) -C deps/luajit/src CC="$(CC)" CXX="$(CXX)" BUILDMODE=static CFLAGS="$(LUA_MYCFLAGS) -w $(fpic)" Q= LDFLAGS="$(LDFLAGS) $(fpic)" libluajit.a

$(INTDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(GVFLAGS) $(MMD) -c -o $@ $<

clean:
	-make -C $(LUADIR) clean
	-rm -f $(OBJS) $(TARGET) $(INTDIR)/$(TARGET)
	if [ -d "obj" ]; then rm -rf obj; fi

test: all
	retroarch --verbose -L lutro_libretro.so test/main.lua

FORCE:


.PHONY: clean
.PHONY: FORCE
