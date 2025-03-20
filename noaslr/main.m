#include <stdio.h>
#include <stdio.h>
#include <dlfcn.h>
#include <unistd.h>
#include "krw.h"
#include "main.h"
#include "kutils.h"
#include "offsets.h"

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

	return KERN_FAILURE;
}

#define P_DISABLE_ASLR  0x00001000      /* Disable address space layout randomization */
int disableASLR(bool disable) {

    uint64_t launchd_proc = proc_of_pid(1);
    uint32_t p_flag = kread32(launchd_proc + off_p_flag);

	if(disable) {
    	kwrite32(launchd_proc + off_p_flag, p_flag | P_DISABLE_ASLR);
		printf("[+] Disabled ASLR.\n");
	}
	else {
		kwrite32(launchd_proc + off_p_flag, p_flag &~P_DISABLE_ASLR);
		printf("[+] Enabled ASLR.\n");
	}

	printf("[*] launchd proc->p_flag: 0x%x\n", p_flag);

    return 0;
}


int main(int argc, char *argv[], char *envp[]) {
	
	offsets_init();

	if(init_kernrw() == KERN_SUCCESS) {
		int r = get_kbase(&kbase);
    	printf("get_kbase ret: %d, kbase: 0x%llx, kslide: 0x%llx\n", r, kbase, kslide);
	}
	// khexdump(kbase, 0x100);

	disableASLR(true);

	return 0;
}
