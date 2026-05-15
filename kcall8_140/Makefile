GO_EASY_ON_ME = 1
DEBUG = 0
FINALPACKAGE = 1

THEOS_DEVICE_IP = idevice

TARGET := iphone:clang:13.7:13.7
ARCHS = arm64

include $(THEOS)/makefiles/common.mk

TOOL_NAME = kcall8

kcall8_FILES = main.m krw.c kutils.c kexecute.c find_port.c offsets.m kcall8.c
kcall8_CFLAGS = -fobjc-arc
kcall8_CODESIGN_FLAGS = -Sentitlements.plist
kcall8_FRAMEWORKS = IOKit
kcall8_INSTALL_PATH = /usr/local/bin

include $(THEOS_MAKE_PATH)/tool.mk
