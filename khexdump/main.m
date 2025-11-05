#include "krw.h"
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

task_t tfp0 = MACH_PORT_NULL;
extern uint64_t kbase;
extern uint64_t kslide;

static kern_return_t
init_tfp0(void) {
	kern_return_t ret = task_for_pid(mach_task_self(), 0, &tfp0);
	mach_port_t host;
	pid_t pid;

	if(ret != KERN_SUCCESS) {
		host = mach_host_self();
		if(MACH_PORT_VALID(host)) {
			printf("host: 0x%x\n", host);
			ret = host_get_special_port(host, HOST_LOCAL_NODE, 4, &tfp0);
			mach_port_deallocate(mach_task_self(), host);
		}
	}
	if(ret == KERN_SUCCESS && MACH_PORT_VALID(tfp0)) {
		if(pid_for_task(tfp0, &pid) == KERN_SUCCESS && pid == 0) {
			return ret;
		}
		mach_port_deallocate(mach_task_self(), tfp0);
	}
	return KERN_FAILURE;
}

void khexdump(uint64_t addr, size_t size) {
    void *data = malloc(size);
    if (!data) return;
    kreadbuf(addr, data, size);

    size_t offset;
    for (offset = 0; offset < size; offset += 16) {
        uint64_t v1 = 0, v2 = 0;
        size_t rem = size - offset;

        size_t n1 = rem >= 8 ? 8 : rem;
        if (n1)
            memcpy(&v1, (unsigned char*)data + offset, n1);

        if (rem > 8) {
            size_t n2 = (rem - 8) >= 8 ? 8 : (rem - 8);
            memcpy(&v2, (unsigned char*)data + offset + 8, n2);
        }

        printf("0x%016llx: 0x%016llx 0x%016llx\n",
               (unsigned long long)(addr + offset),
               (unsigned long long)v1,
               (unsigned long long)v2);
    }

    free(data);
}


int main(int argc, char *argv[], char *envp[]) {

	if(init_tfp0() == KERN_SUCCESS) {
		printf("tfp0: 0x%x\n", tfp0);

		int r = get_kbase(&kbase);
    	printf("get_kbase ret: %d, kbase: 0x%llx\n", r, kbase);

		uint64_t addr = kbase;
		size_t   size = 0x100;

		if (argc >= 2) {
			addr = (uint64_t)strtoull(argv[1], NULL, 0);
		}
		if (argc >= 3) {
			size = (size_t)strtoull(argv[2], NULL, 0);
		}
		khexdump(addr, size);

		mach_port_deallocate(mach_task_self(), tfp0);
	}
}
