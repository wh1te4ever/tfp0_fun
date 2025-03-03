#include <stdio.h>
#include <dlfcn.h>
#include "main.h"
#include "krw.h"
#include "offsets.h"
#include "kalloc_pipe.h"
#include "kexecute.h"
#include "kcall8.h"

task_t tfp0 = MACH_PORT_NULL;
void *libkernrw = NULL;
extern uint64_t kbase;
extern uint64_t kslide;

static kern_return_t
init_kernrw(void) {
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

	libkernrw = dlopen("/usr/lib/libkernrw.0.dylib", RTLD_NOW);
	if(libkernrw != NULL) {
		void *requestKernRw_ptr = dlsym(libkernrw, "requestKernRw");
		if(requestKernRw_ptr != NULL) {
			int (*requestKernRw)(void) = NULL;
    		requestKernRw = (int (*)(void))requestKernRw_ptr;
			if(requestKernRw() == 0)
    			return KERN_SUCCESS;
		}
	}

	return KERN_FAILURE;
}



int main(int argc, char *argv[], char *envp[]) {

	offsets_init();

	int r = init_kernrw();
	printf("init_kernrw r = 0x%x\n", r);

	r = get_kbase(&kbase);
    printf("get_kbase ret: %d, kbase: 0x%llx, kslide: 0x%llx\n", r, kbase, kslide);

	//kalloc_via_pipe test
	pipe_info_t *fakePipeAlloc = kalloc_via_pipe(0x4000);
	uint64_t kptr = fakePipeAlloc->kern_buffer;
	// khexdump(kptr, 0x1000);
	kfree_via_pipe(fakePipeAlloc);

	//kexecute, kcall8 test
	init_kexecute();

	uint64_t kret = kexecute(ksym(KSYMBOL_RET_300), 1, 0, 0, 0, 0, 0, 0);
	printf("kexecute KSYMBOL_RET_300 kret = %llu\n", kret);

	const uint64_t kalloc_sz = 0x1000;
	kptr = kcall8(ksym(KSYMBOL_KALLOC_EXTERNAL), kalloc_sz, 0, 0, 0, 0, 0, 0, 0);
	printf("kcall8 KSYMBOL_KALLOC_EXTERNAL(0x%llx) kptr = 0x%llx\n", kalloc_sz, kptr);
	kret = kcall8(ksym(KSYMBOL_KFREE), kptr, kalloc_sz, 0, 0, 0, 0, 0, 0);
	printf("kcall8 KSYMBOL_KFREE kret = 0x%llx\n", kret);


	term_kexecute();

	return 0;
}
