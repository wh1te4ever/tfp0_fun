GO_EASY_ON_ME = 1
DEBUG = 0
FINALPACKAGE = 1

THEOS_DEVICE_IP = 172.30.1.87 -p 2222

TARGET := iphone:clang:latest:7.0
ARCHS = arm64

include $(THEOS)/makefiles/common.mk

TOOL_NAME = IOSurfacePrimitive

IOSurfacePrimitive_FILES = main.m krw.c IOSurfaceRoot.m piper.c offsets.m kutils.c find_port.c kprimitive.c kalloc_pipe.c
IOSurfacePrimitive_CFLAGS = -fobjc-arc
IOSurfacePrimitive_FRAMEWORKS = IOKit
IOSurfacePrimitive_CODESIGN_FLAGS = -Sentitlements.plist
IOSurfacePrimitive_INSTALL_PATH = /usr/local/bin

include $(THEOS_MAKE_PATH)/tool.mk
