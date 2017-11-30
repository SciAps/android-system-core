LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)


LOCAL_MODULE := fbtest
LOCAL_SRC_FILES := \
  test.c
LOCAL_STATIC_LIBRARIES := libcutils libc
include $(BUILD_EXECUTABLE)
