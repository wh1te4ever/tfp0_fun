GO_EASY_ON_ME = 1
DEBUG = 0
FINALPACKAGE = 1

THEOS_DEVICE_IP = 127.0.0.1 -p 2222

TARGET := iphone:clang:latest:7.0
ARCHS = arm64

include $(THEOS)/makefiles/common.mk

TOOL_NAME = i41ndbg

i41ndbg_FILES = main.m krw.c offsets.m kutils.c find_port.c early_kalloc.c kcall.c kexecute.c kdbg.c 
i41ndbg_CFLAGS = -fobjc-arc 
i41ndbg_CODESIGN_FLAGS = -Sentitlements.plist
i41ndbg_FRAMEWORKS = IOKit
i41ndbg_INSTALL_PATH = /usr/local/bin

include $(THEOS_MAKE_PATH)/tool.mk
