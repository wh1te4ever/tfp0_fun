#include "kextrw.h"
#include "offsets.h"
#include <stdint.h>
#include <mach/mach.h> 
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <IOKit/IOKitLib.h>
#include <mach-o/loader.h>

uint64_t gKernelBase = 0, gKernelSlide = 0;
io_connect_t gClient = MACH_PORT_NULL;

#define VM_KERNEL_LINK_ADDR 0xFFFFFE0007004000ULL


#ifndef MIN
#    define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

kern_return_t kextrw_init(void) {
    io_service_t service = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("KextRW"));
    if(!MACH_PORT_VALID(service))   return KERN_FAILURE;
    kern_return_t ret = IOServiceOpen(service, mach_task_self(), 0, &gClient);
    printf("gClient=0x%llx\n", gClient);
    IOObjectRelease(service);
    return KERN_SUCCESS;
}

void kextrw_deinit(void) {
    IOServiceClose(gClient);
}

static inline kern_return_t kextrw_get_reset_vector(io_connect_t client, uint64_t *out)
{
    uint32_t outCnt = 1; 
    return IOConnectCallScalarMethod(client, 4, NULL, 0, out, &outCnt);
}

int kextrw_get_kernel_base(void)
{
    uint64_t kernelPage = 0;
    // printf("gClient = 0x%llx\n", gClient);
    kextrw_get_reset_vector(gClient, &kernelPage);
    // printf("kernelPage: 0x%llx\n", kernelPage);
    if (!kernelPage) return 0;

    uint64_t kernelBase = 0;
    while (!kernelBase) {
        if (kextrw_kread32(kernelPage) == MH_MAGIC_64
            && kextrw_kread32(kernelPage + 0xC) == MH_EXECUTE) {
            kernelBase = kernelPage;
            break;
        }
        kernelPage -= 0x1000;
    }

    gKernelSlide = kernelBase - VM_KERNEL_LINK_ADDR;
    gKernelBase = kernelBase;

    printf("gKernelSlide = 0x%llx, gKernelBase = 0x%llx\n", gKernelSlide, gKernelBase);

    return 0;
}

kern_return_t
kextrw_kreadbuf(uint64_t kaddr, void *buf, size_t sz) {
    uint64_t in[] = { kaddr, (uint64_t)buf, sz };
    return IOConnectCallScalarMethod(gClient, 0, in, 3, NULL, NULL);
}

kern_return_t
kextrw_kwritebuf(uint64_t kaddr, const void *buf, size_t sz) {
    if (!buf || sz == 0) return KERN_SUCCESS;

    const uint8_t *src = (const uint8_t *)buf;
    uint64_t remaining = sz;
    uint64_t offset = 0;

    while (remaining != 0) {
        uint64_t writeSize = (remaining >= sizeof(uint32_t)) 
                             ? sizeof(uint32_t) 
                             : remaining;
                             
        uint64_t writeDst = kaddr + offset;

        if (writeSize != sizeof(uint32_t)) {
            // Tail fragment < 4 bytes
            uint64_t adjust = 0;
            
            // Shift backward if possible to prevent reading past boundary
            if (offset >= (sizeof(uint32_t) - writeSize)) {
                adjust = sizeof(uint32_t) - writeSize;
                writeDst -= adjust;
            }

            // Read-Modify-Write using the shifted address
            uint32_t val = (uint32_t)(kextrw_kread64(writeDst) & 0xFFFFFFFF);
            memcpy(((uint8_t *)&val) + adjust, src + offset, writeSize);
            kextrw_kwrite32(writeDst, val);
        } else {
            // Bulk write 4-byte blocks
            uint32_t val;
            memcpy(&val, src + offset, sizeof(uint32_t));
            kextrw_kwrite32(writeDst, val);
        }

        remaining -= writeSize;
        offset    += writeSize;
    }

    return KERN_SUCCESS;
}

uint8_t kextrw_kread8(uint64_t where) {
    uint8_t out;
    kextrw_kreadbuf(where, &out, sizeof(uint8_t));
    return out;
}

uint16_t kextrw_kread16(uint64_t where) {
    uint16_t out;
    kextrw_kreadbuf(where, &out, sizeof(uint16_t));
    return out;
}

uint32_t kextrw_kread32(uint64_t where) {
    uint32_t out;
    kextrw_kreadbuf(where, &out, sizeof(uint32_t));
    return out;
}

uint64_t kextrw_kread64(uint64_t where) {
    uint64_t out;
    kextrw_kreadbuf(where, &out, sizeof(uint64_t));
    return out;
}

uint64_t xpaci(uint64_t ptr)
{
	asm("xpaci %[value]\n" : [value] "+r"(ptr));
	return ptr;
}

void kextrw_kwrite8(uint64_t where, uint8_t what) {
    uint64_t _what = kextrw_kread64(where);
    kextrw_kwrite32(where, (_what & 0xFFFFFF00) | (uint32_t)what);
}

void kextrw_kwrite16(uint64_t where, uint16_t what) {
    uint64_t _what = kextrw_kread64(where);
    kextrw_kwrite32(where, (_what & 0xFFFF0000) | (uint32_t)what);
}

void kextrw_kwrite32(uint64_t where, uint32_t what) {
    kextrw_kcall(ksym(KSYMBOL_STR_W1_X0_RET), (uint64_t []){ where, what,  }, 2);
}

void kextrw_kwrite64(uint64_t where, uint64_t what) {
    kextrw_kcall(ksym(KSYMBOL_STR_X1_X0_RET), (uint64_t []){ where, what,  }, 2);
}

void kextrw_khexdump(uint64_t addr, size_t size) {
    void *data = malloc(size);
    kextrw_kreadbuf(addr, data, size);
    char ascii[17];
    size_t i, j;
    ascii[16] = '\0';
    for (i = 0; i < size; ++i) {
        if ((i % 16) == 0)
        {
            printf("[0x%016llx+0x%03zx] ", addr, i);
//            printf("[0x%016llx] ", i + addr);
        }
        
        printf("%02X ", ((unsigned char*)data)[i]);
        if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
            ascii[i % 16] = ((unsigned char*)data)[i];
        } else {
            ascii[i % 16] = '.';
        }
        if ((i+1) % 8 == 0 || i+1 == size) {
            printf(" ");
            if ((i+1) % 16 == 0) {
                printf("|  %s \n", ascii);
            } else if (i+1 == size) {
                ascii[(i+1) % 16] = '\0';
                if ((i+1) % 16 <= 8) {
                    printf(" ");
                }
                for (j = (i+1) % 16; j < 16; ++j) {
                    printf("   ");
                }
                printf("|  %s \n", ascii);
            }
        }
    }
    free(data);
}

static inline kern_return_t kextrw_kcall_internal(io_connect_t client, uint64_t fn, uint64_t *args, uint32_t argsCnt, uint64_t *out)
{
    uint64_t argsBuf[11] = { 0 };
    argsBuf[0] = fn;
    for (uint32_t i = 0; i < argsCnt; i++)
    {
        if (args[i]) argsBuf[i + 1] = args[i] ? args[i] : 0;
    }
    uint32_t outCnt = 1;
    uint64_t rv = 0;
    IOReturn ret = IOConnectCallScalarMethod(client, 7, argsBuf, 11, &rv, &outCnt);
    if (out) *out = rv;
    return ret;
}

uint64_t kextrw_kcall(uint64_t fn, uint64_t *args, uint32_t argsCnt)
{
    uint64_t rv = 0;
    if (argsCnt > 10) return KERN_INVALID_ARGUMENT;
    kern_return_t kr = kextrw_kcall_internal(gClient, fn, args, argsCnt, &rv);
    if (kr != KERN_SUCCESS) printf("WARNING: kcall failed with error %d\n", kr);
    return rv;
}

uint64_t kextrw_kreadptr(uint64_t addr)
{
    return xpaci(kextrw_kread64(addr));
}

uint64_t kextrw_kvtophys(uint64_t va)
{
    uint64_t pa = 0;
    uint32_t outCnt = 1;
    IOConnectCallScalarMethod(gClient, 5, &va, 1, &pa, &outCnt);
    return pa;
}

uint64_t kextrw_phystokv(uint64_t pa)
{
    uint64_t va = 0;
    uint32_t outCnt = 1;
    IOConnectCallScalarMethod(gClient, 6, &pa, 1, &va, &outCnt);
    return va;
}

uint64_t kextrw_kalloc(uint64_t size)
{
    uint64_t addr = 0;
    uint32_t outCnt = 1;
    IOConnectCallScalarMethod(gClient, 8, &size, 1, &addr, &outCnt);
    return addr;
}

kern_return_t kextrw_kfree(uint64_t addr, uint64_t size)
{
    uint64_t in[] = { addr, size };
    return IOConnectCallScalarMethod(gClient, 9, in, 2, NULL, NULL);
}