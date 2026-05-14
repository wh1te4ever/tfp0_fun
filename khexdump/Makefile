GO_EASY_ON_ME = 1
DEBUG = 0
FINALPACKAGE = 1

THEOS_DEVICE_IP = idevice

TARGET := iphone:clang:latest:7.0
ARCHS = arm64

include $(THEOS)/makefiles/common.mk

TOOL_NAME = khexdump

khexdump_FILES = main.m krw.c
khexdump_CFLAGS = -fobjc-arc
khexdump_CODESIGN_FLAGS = -Sentitlements.plist
khexdump_INSTALL_PATH = /usr/local/bin

include $(THEOS_MAKE_PATH)/tool.mk
