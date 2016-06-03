#
# Android.mk
# Emiliano Firmino, 2015-11-17 19:28
# minor changes by Andrea Scian
#

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := cpulimit

LOCAL_SRC_FILES := src/cpulimit.c \
	src/list.c \
	src/memrchr.c \
	src/process_iterator.c \
	src/process_group.c

include $(BUILD_EXECUTABLE)

# vim:ft=make
#
