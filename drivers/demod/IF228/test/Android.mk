LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := demodtest
LOCAL_SRC_FILES := test.c

include $(BUILD_EXECUTABLE)

include $(call all-makefiles-under,$(LOCAL_PATH))