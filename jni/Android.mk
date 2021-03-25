LOCAL_PATH := $(call my-dir)

CORE_DIR    := $(LOCAL_PATH)/..
WANT_UNZIP  := 1
WANT_LUALIB := 1

include $(CORE_DIR)/Makefile.common

COREFLAGS := -ffast-math -funroll-loops -DINLINE=inline -D__LIBRETRO__ -DFRONTEND_SUPPORTS_RGB565 -DLUA_USE_POSIX $(INCFLAGS) -DDONT_WANT_ARM_OPTIMIZATIONS

GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
  COREFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

include $(CLEAR_VARS)
LOCAL_MODULE    := retro
LOCAL_SRC_FILES := $(SOURCES_C) $(VORBIS_SOURCES_C)
LOCAL_CFLAGS    := $(COREFLAGS) -std=gnu99
LOCAL_CXXFLAGS  := $(COREFLAGS)
LOCAL_LDFLAGS   := -Wl,-version-script=$(CORE_DIR)/link.T
LOCAL_LDLIBS    := -lz

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
  LOCAL_ARM_NEON := true
endif

include $(BUILD_SHARED_LIBRARY)
