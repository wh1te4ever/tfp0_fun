#include "main.h"
#include "krw.h"
#include "offsets.h"
#include "kutils.h"
#include "kexecute.h"
#include "kcall8.h"

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

		// uint64_t our_task = task_self_addr();
		// printf("our_task = 0x%llx\n", our_task);

		init_kexecute();

		//kexecute works
		uint64_t kret = kexecute(ksym(KSYMBOL_RET_300), 1, 0, 0, 0, 0, 0, 0);
		printf("kexecute KSYMBOL_RET_300 kret = %llu\n", kret);

		//kcall8 works
		kret = kcall8(ksym(KSYMBOL_RET_300), 1, 0, 0, 0, 0, 0, 0, 0);
		printf("kcall8 KSYMBOL_RET_300 kret = %llu\n", kret);

		//kcall8 panic with x1-x7 
		const uint64_t kalloc_sz = 0x1000;
		uint64_t kptr = kcall8(ksym(KSYMBOL_KALLOC_EXTERNAL), kalloc_sz, 0, 0, 0, 0, 0, 0, 0);
		printf("kcall8 KSYMBOL_KALLOC_EXTERNAL(0x%llx) kptr = 0x%llx\n", kalloc_sz, kptr);

		//br x3, = 0xdeadbeef41424343 invalid pc  
		// kcall8(kgad(KGADGET_MOV_X15_X2__BR_X3) + 4, 0xdeadbeef41424340, 0xdeadbeef41424341, 0xdeadbeef41424342, 0xdeadbeef41424343, 0xdeadbeef41424344, 0xdeadbeef41424345, 0xdeadbeef41424346, 0xdeadbeef41424347);
		
		kret = kcall8(ksym(KSYMBOL_KFREE), kptr, kalloc_sz, 0, 0, 0, 0, 0, 0);
		printf("kcall8 KSYMBOL_KFREE kret = 0x%llx\n", kret);

		khexdump(kptr, kalloc_sz+0x10);

		term_kexecute();

		mach_port_deallocate(mach_task_self(), tfp0);
	}
}
