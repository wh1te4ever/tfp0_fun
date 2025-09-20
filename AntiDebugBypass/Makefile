GO_EASY_ON_ME = 1
DEBUG = 0
FINALPACKAGE = 1
TARGET := iphone:clang:12.4:12.4

ARCHS = arm64

THEOS_PACKAGE_SCHEME = rootless
THEOS_DEVICE_IP = 127.0.0.1 -p 2222

include $(THEOS)/makefiles/common.mk

TOOL_NAME = AntiDebugBypass

AntiDebugBypass_FILES = main.m krw.c
AntiDebugBypass_CFLAGS = -fobjc-arc
AntiDebugBypass_CODESIGN_FLAGS = -Sentitlements.plist
AntiDebugBypass_FRAMEWORKS = IOKit
AntiDebugBypass_INSTALL_PATH = /usr/local/bin

include $(THEOS_MAKE_PATH)/tool.mk
