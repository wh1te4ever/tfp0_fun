#include <stdio.h>
#include <dlfcn.h>
#include "main.h"
#include "krw.h"
#include "offsets.h"
#include "kalloc_pipe.h"
#include "kexecute.h"
#include "kcall8.h"
#include "physrw.h"
#include "kfunc.h"
#include "translation.h"

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
	translation_init();

	if(init_kernrw() == KERN_SUCCESS) {
		int r = get_kbase(&kbase);
    	printf("get_kbase ret: %d, kbase: 0x%llx, kslide: 0x%llx\n", r, kbase, kslide);

		//kexecute, kcall8 test
		init_kexecute();

		uint64_t kret = kexecute(ksym(KSYMBOL_RET_300), 1, 0, 0, 0, 0, 0, 0);
		printf("kexecute KSYMBOL_RET_300 kret = %llu\n", kret);

		//kernel mapping with phys r/w
		physrw_handoff(getpid());

		const uint64_t kalloc_sz = 0x100;
		uint64_t kptr = kcall8(ksym(KSYMBOL_KALLOC_EXTERNAL), kalloc_sz, 0, 0, 0, 0, 0, 0, 0);
		printf("kcall8 KSYMBOL_KALLOC_EXTERNAL(0x%llx) kptr = 0x%llx\n", kalloc_sz, kptr);

		kwrite64_phys(kptr, 0xCAFEBABE41424344);
		printf("kread64_phys(kbase) = 0x%llx\n", kread64_phys(kbase));
		printf("kread64_phys(kptr) = 0x%llx\n", kread64_phys(kptr));
		

		kret = kcall8(ksym(KSYMBOL_KFREE), kptr, kalloc_sz, 0, 0, 0, 0, 0, 0);
		printf("kcall8 KSYMBOL_KFREE kret = 0x%llx\n", kret);

		// kret = kfunc_kvtophys(kbase);
		// printf("kfunc_kvtophys kret = 0x%llx\n", kret);
		// printf("physread64 = 0x%llx\n", physread64(kret));

		// kret = kvtophys(kbase);
		// printf("kvtophys kret = 0x%llx\n", kret);
		// printf("physread64 = 0x%llx\n", physread64(kret));

		term_kexecute();

		printf("done\n");
		// while(1) {};
	}

	return 0;
}
