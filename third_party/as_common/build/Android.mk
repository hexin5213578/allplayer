LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CFLAGS += -DOS_LINUX -DOS_ANDROID  -pipe -fPIC -g -Wall -O0 -DSOCKLEN_T=socklen_t -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE
LOCAL_LDLIBS += -lm -pthread -ldl

LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../inc

LOCAL_SRC_FILES := $(wildcard ../src/*.c)
LOCAL_SRC_FILES += $(wildcard ../src/*.cpp)

LOCAL_MODULE := common
include $(BUILD_SHARED_LIBRARY)
