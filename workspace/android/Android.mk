### --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES := bionic
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include
LOCAL_CFLAGS += -Wall -O2 -std=c++11
LOCAL_LDLIBS := -L$(LOCAL_PATH)/lib -llog
# /boost//system /boost//chrono

LOCAL_SRC_FILES := hgs-rtph264.cpp hgs-jni.cpp VideoRender.cpp
LOCAL_STATIC_LIBRARIES :=
LOCAL_MODULE := hgs
include $(BUILD_SHARED_LIBRARY)

### --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---
# include $(CLEAR_VARS)
# LOCAL_MODULE    := hgs
# 
# LOCAL_SRC_FILES := armeabi-v7a/libhgs.so
# include $(PREBUILT_SHARED_LIBRARY)
# 
# #LOCAL_SRC_FILES := armeabi-v7a/libhgs.a
# #include $(PREBUILT_STATIC_LIBRARY)
#

### --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---
#include $(BUILD_EXECUTABLE)
#// ifneq ($(TARGET_SIMULATOR),true) ### TARGET_SIMULATOR != true
#
#// endif  # TARGET_SIMULATOR != true

