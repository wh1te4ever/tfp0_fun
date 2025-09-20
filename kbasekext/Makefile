GO_EASY_ON_ME = 1
DEBUG = 0
FINALPACKAGE = 1

THEOS_DEVICE_IP = 172.30.1.87 -p 22

TARGET := iphone:clang:latest:7.0
ARCHS = arm64

include $(THEOS)/makefiles/common.mk

TOOL_NAME = kbasekext

kbasekext_FILES = main.m krw.c kutils.c offsets.m
kbasekext_CFLAGS = -fobjc-arc
kbasekext_FRAMEWORKS = IOKit
kbasekext_CODESIGN_FLAGS = -Sentitlements.plist
kbasekext_INSTALL_PATH = /usr/local/bin

include $(THEOS_MAKE_PATH)/tool.mk
