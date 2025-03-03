TARGET := iphone:clang:latest:7.0

include $(THEOS)/makefiles/common.mk

TOOL_NAME = physmappy

physmappy_FILES = main.m
physmappy_CFLAGS = -fobjc-arc
physmappy_CODESIGN_FLAGS = -Sentitlements.plist
physmappy_INSTALL_PATH = /usr/local/bin

include $(THEOS_MAKE_PATH)/tool.mk
