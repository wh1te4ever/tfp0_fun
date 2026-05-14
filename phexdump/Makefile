GO_EASY_ON_ME = 1
DEBUG = 0
FINALPACKAGE = 1

THEOS_DEVICE_IP = 127.0.0.1 -p 22280

TARGET := iphone:clang:latest:7.0
ARCHS = arm64
THEOS_PACKAGE_SCHEME = rootless

include $(THEOS)/makefiles/common.mk

TOOL_NAME = phexdump

phexdump_FILES = main.m 
phexdump_FILES += helper/uhexdump.c helper/offsets.c helper/proc.c helper/tfp0_krw.c helper/tfp0_find_port.c helper/find_IOSurface.c helper/kcall8.c helper/kexecute.c helper/find_pipe.c helper/translation.c helper/pmap.c helper/vram.m		#helper
phexdump_CFLAGS = -fobjc-arc
phexdump_CODESIGN_FLAGS = -Sentitlements.plist
phexdump_INSTALL_PATH = /usr/local/bin
phexdump_FRAMEWORKS = IOSurface IOKit

include $(THEOS_MAKE_PATH)/tool.mk
