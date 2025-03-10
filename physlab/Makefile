GO_EASY_ON_ME = 1
DEBUG = 0
FINALPACKAGE = 1

THEOS_DEVICE_IP = 127.0.0.1 -p 2222

TARGET := iphone:clang:latest:7.0
ARCHS = arm64

include $(THEOS)/makefiles/common.mk

TOOL_NAME = physlab

physlab_FILES = main.m krw.c kalloc_pipe.c offsets.m kutils.c kexecute.c find_port.c kcall8.c physrw.m pmap.c translation.c kfunc.c
physlab_CFLAGS = -fobjc-arc
physlab_FRAMEWORKS = IOKit
physlab_CODESIGN_FLAGS = -Sentitlements.plist
physlab_INSTALL_PATH = /usr/local/bin

include $(THEOS_MAKE_PATH)/tool.mk
