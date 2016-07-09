LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:=\
	demo.cpp
	#a.cpp

LOCAL_CFLAGS:=-O2 -g -std=c++11
#LOCAL_CFLAGS+=-DLINUX

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)

LOCAL_MODULE:=a

LOCAL_SHARED_LIBRARIES += libutils #libstagefright liblog
#LOCAL_SHARED_LIBRARIES += libssl libcrypto liblog libsysutils libcutils

# gold in binutils 2.22 will warn about the usage of mktemp
LOCAL_LDFLAGS += -Wl,--no-fatal-warnings
LOCAL_LDFLAGS += -L$(LOCAL_PATH)/system/lib \
	-lstagefright_omx -lstagefright -lstagefright_foundation -lcutils -lutils -lbinder -lui -lgui -landroid_runtime -llog

#LOCAL_CPPFLAGS := -Wall -DHAVE_PTHREADS -I$(LOCAL_PATH)/include -I$(LOCAL_PATH)/include/openmax -I$(LOCAL_PATH)/inc/native
LOCAL_C_INCLUDES += \
	frameworks/native/include/media/openmax \
	frameworks/native/include/media/hardware
	#frameworks/av/media/libstagefright

include external/stlport/libstlport.mk
include $(BUILD_EXECUTABLE)

