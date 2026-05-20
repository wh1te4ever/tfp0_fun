#!/bin/zsh

rm inpcbKernelRW;

xcrun -sdk macosx clang\
    -arch arm64 \
    -Weverything main.c kextrw.c offsets.c kutils.c \
    -w -O0 \
    -o inpcbKernelRW -framework \
    IOKit -framework \
    IOSurface -framework \
    Foundation -framework \
    CoreFoundation -lcompression -O0

ldid -S./ents.plist ./inpcbKernelRW;