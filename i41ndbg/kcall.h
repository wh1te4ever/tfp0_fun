#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <CoreFoundation/CoreFoundation.h>
#include <mach/mach.h>
#include <stdint.h>

typedef mach_port_t io_object_t;
typedef io_object_t io_connect_t, io_service_t;

kern_return_t
IOObjectRelease(io_object_t);

kern_return_t
IOServiceClose(io_connect_t);

CFMutableDictionaryRef
IOServiceMatching(const char *);

io_service_t
IOServiceGetMatchingService(mach_port_t, CFDictionaryRef);

kern_return_t
IOServiceOpen(io_service_t, task_port_t, uint32_t, io_connect_t *);


uint64_t kcall(uint64_t fptr, uint32_t argc, ...);
void test_kcall();
void test_kcall2();

uint64_t IOConnectTrap6(io_connect_t, uint32_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);