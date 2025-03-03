TARGET := iphone:clang:latest:7.0

include $(THEOS)/makefiles/common.mk

TOOL_NAME = earlykalloc

earlykalloc_FILES = main.m
earlykalloc_CFLAGS = -fobjc-arc
earlykalloc_CODESIGN_FLAGS = -Sentitlements.plist
earlykalloc_INSTALL_PATH = /usr/local/bin

include $(THEOS_MAKE_PATH)/tool.mk
