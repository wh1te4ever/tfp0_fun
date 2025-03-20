GO_EASY_ON_ME = 1
DEBUG = 0
FINALPACKAGE = 1

THEOS_DEVICE_IP = 127.0.0.1 -p 2223

TARGET := iphone:clang:latest:7.0
ARCHS = arm64

include $(THEOS)/makefiles/common.mk

TOOL_NAME = noaslr

noaslr_FILES = main.m krw.c kutils.c offsets.m
noaslr_CFLAGS = -fobjc-arc
noaslr_FRAMEWORKS = IOKit
noaslr_CODESIGN_FLAGS = -Sentitlements.plist
noaslr_INSTALL_PATH = /usr/local/bin

include $(THEOS_MAKE_PATH)/tool.mk
