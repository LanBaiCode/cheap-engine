LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_DISABLE_FATAL_LINKER_WARNINGS := true
LOCAL_MODULE    := 4ntich3at
LOCAL_SRC_FILES := cheap-engine.cpp speedhack.cpp
LOCAL_LDLIBS := -llog
include $(BUILD_SHARED_LIBRARY)