TARGET := iphone:clang:latest:7.0

include $(THEOS)/makefiles/common.mk

TOOL_NAME = IOSurfacePrimitive

IOSurfacePrimitive_FILES = main.m
IOSurfacePrimitive_CFLAGS = -fobjc-arc
IOSurfacePrimitive_CODESIGN_FLAGS = -Sentitlements.plist
IOSurfacePrimitive_INSTALL_PATH = /usr/local/bin

include $(THEOS_MAKE_PATH)/tool.mk
