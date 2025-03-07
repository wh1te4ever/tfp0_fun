#include <stdio.h>
#include <mach-o/loader.h>
#include <mach/mach.h>
#include <unistd.h>
#include <inttypes.h>
#include <errno.h>
#include <stdlib.h> 
#include <unistd.h>

#include "main.h"
#include "krw.h"
#include "offsets.h"
#include "kdbg.h"
#include "kutils.h"
#include "kcall.h"
#include "kexecute.h"
#include "kdp_server.h"
#include "kernel_debug_me.h"

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
			printf("host: 0x%" PRIx32 "\n", host);
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

int main(int argc, char *argv[], char *envp[]) {

	offsets_init();

	if(init_tfp0() == KERN_SUCCESS) {
		printf("tfp0: 0x%" PRIx32 "\n", tfp0);

    	int r = get_kbase(&kbase);
    	printf("get_kbase ret: %d, kbase: 0x%llx, kslide: 0x%llx\n", r, kbase, kslide);

		// init_kexecute();

		// // printf("init_kexec seems working\n");
		// // kexecute();
		// uint64_t kret = kexecute(0xFFFFFFF0062613E4 + kslide, 1, 0, 0, 0, 0, 0, 0);
		// printf("kret = 0x%llx\n", kret);
		// // sleep(1);

		// term_kexecute();

		// uint64_t kptr = kalloc(0x100);
		// khexdump(kptr-0x60, 0x100);
		// khexdump(ksym(KSYMBOL_X21_JOP_GADGET), 0x170);
		// kfree(kptr, 0x100);
		// khexdump(kptr, 0x100);

		printf("starting kdp server\n");
    	start_kdp_server();

		// run the iokit PoC to test under kdp:
		kernel_debug_me();

		mach_port_deallocate(mach_task_self(), tfp0);
	}
}
