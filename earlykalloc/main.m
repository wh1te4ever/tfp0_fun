#include <stdio.h>
#include <dlfcn.h>
#include "main.h"
#include "krw.h"

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

	int r = init_kernrw();
	printf("init_kernrw r = 0x%x\n", r);

	r = get_kbase(&kbase);
    printf("get_kbase ret: %d, kbase: 0x%llx, kslide: 0x%llx\n", r, kbase, kslide);

	khexdump(kbase, 0x100);

	return 0;
}
