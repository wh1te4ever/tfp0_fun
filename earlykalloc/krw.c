#include "krw.h"
#include <stdint.h>
#include <mach/mach.h> 
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

extern task_t tfp0;
extern void* libkernrw;
uint64_t kbase = 0;
uint64_t kslide = 0;

#ifndef MIN
#    define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

int get_kbase(uint64_t *addr)
{
    if(tfp0 == MACH_PORT_NULL)
    {
        void *kernRW_getKernelBase_ptr = dlsym(libkernrw, "kernRW_getKernelBase");
		if(kernRW_getKernelBase_ptr != NULL) {
			kern_return_t (*kernRW_getKernelBase)(uint64_t *) = NULL;
    		kernRW_getKernelBase = (kern_return_t (*)(uint64_t *))kernRW_getKernelBase_ptr;

            uint64_t kbase = 0;
			kernRW_getKernelBase(&kbase);
            *addr = kbase;
            kslide = kbase - 0xfffffff007004000;

            return 0;
		}
        return ENOTSUP;
    }

    task_dyld_info_data_t info = {};
    uint32_t count = TASK_DYLD_INFO_COUNT;
    kern_return_t ret = task_info(tfp0, TASK_DYLD_INFO, (task_info_t)&info, &count);
    if(ret != KERN_SUCCESS)
    {
        return EDEVERR;
    }
	
    if(info.all_image_info_addr == 0 && info.all_image_info_size == 0)
    {
        return ENOTSUP;
    }
    kslide = info.all_image_info_size;
    *addr = 0xfffffff007004000 + info.all_image_info_size;
    return 0;
}

kern_return_t
kreadbuf(uint64_t kaddr, void *buf, size_t sz) {
    if(tfp0 == MACH_PORT_NULL) {
        void *kernRW_readbuf_ptr = dlsym(libkernrw, "kernRW_readbuf");
		if(kernRW_readbuf_ptr != NULL) {
			kern_return_t (*kernRW_readbuf)(uint64_t, void *, size_t) = NULL;
    		kernRW_readbuf = (kern_return_t (*)(uint64_t, void *, size_t))kernRW_readbuf_ptr;
            return kernRW_readbuf(kaddr, buf, sz);
		}
        return KERN_FAILURE;
    }

    mach_vm_address_t p = (mach_vm_address_t)buf;
    mach_vm_size_t read_sz, out_sz = 0;

    while(sz != 0) {
        read_sz = MIN(sz, vm_kernel_page_size - (kaddr & vm_kernel_page_mask));
        if(mach_vm_read_overwrite(tfp0, kaddr, read_sz, p, &out_sz) != KERN_SUCCESS || out_sz != read_sz) {
            return KERN_FAILURE;
        }
        p += read_sz;
        sz -= read_sz;
        kaddr += read_sz;
    }
    return KERN_SUCCESS;
}

kern_return_t
kwritebuf(uint64_t kaddr, const void *buf, size_t sz) {
    if(tfp0 == MACH_PORT_NULL) {
        void *kernRW_writebuf_ptr = dlsym(libkernrw, "kernRW_writebuf");
		if(kernRW_writebuf_ptr != NULL) {
			kern_return_t (*kernRW_writebuf)(uint64_t, const void *, size_t) =  NULL;
    		kernRW_writebuf = (kern_return_t (*)(uint64_t, const void *, size_t))kernRW_writebuf_ptr;
            return kernRW_writebuf(kaddr, buf, sz);
		}
        return KERN_FAILURE;
    }

    vm_machine_attribute_val_t mattr_val = MATTR_VAL_CACHE_FLUSH;
    mach_vm_address_t p = (mach_vm_address_t)buf;
    mach_msg_type_number_t write_sz;

    while(sz != 0) {
        write_sz = (mach_msg_type_number_t)MIN(sz, vm_kernel_page_size - (kaddr & vm_kernel_page_mask));
        if(mach_vm_write(tfp0, kaddr, p, write_sz) != KERN_SUCCESS || mach_vm_machine_attribute(tfp0, kaddr, write_sz, MATTR_CACHE, &mattr_val) != KERN_SUCCESS) {
            return KERN_FAILURE;
        }
        p += write_sz;
        sz -= write_sz;
        kaddr += write_sz;
    }
    return KERN_SUCCESS;
}

uint32_t kread32(uint64_t where) {
    uint32_t out;
    kreadbuf(where, &out, sizeof(uint32_t));
    return out;
}

uint64_t kread64(uint64_t where) {
    uint64_t out;
    kreadbuf(where, &out, sizeof(uint64_t));
    return out;
}

void kwrite32(uint64_t where, uint32_t what) {
    uint32_t _what = what;
    kwritebuf(where, &_what, sizeof(uint32_t));
}

void kwrite64(uint64_t where, uint64_t what) {
    uint64_t _what = what;
    kwritebuf(where, &_what, sizeof(uint64_t));
}

const uint64_t kernel_address_space_base = 0xffff000000000000;
void kmemcpy(uint64_t dest, uint64_t src, uint32_t length) {
    if (dest >= kernel_address_space_base) {
      // copy to kernel:
      kwritebuf(dest, (void*) src, length);
    } else {
      // copy from kernel
      kreadbuf(src, (void*)dest, length);
    }
}

void khexdump(uint64_t addr, size_t size) {
    void *data = malloc(size);
    kreadbuf(addr, data, size);
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