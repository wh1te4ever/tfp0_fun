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


typedef enum zalloc_flags {
    // values smaller than 0xff are shared with the M_* flags from BSD MALLOC
	Z_WAITOK        = 0x0000,
	Z_NOWAIT        = 0x0001,
	Z_NOPAGEWAIT    = 0x0002,
	Z_ZERO          = 0x0004,

	Z_NOFAIL        = 0x8000,

	/* convenient c++ spellings */
	Z_NOWAIT_ZERO          = Z_NOWAIT | Z_ZERO,
	Z_WAITOK_ZERO          = Z_WAITOK | Z_ZERO,
	Z_WAITOK_ZERO_NOFAIL   = Z_WAITOK | Z_ZERO | Z_NOFAIL, /* convenient spelling for c++ */

	Z_NOZZC         = 0x4000,

	/** used by kalloc to propagate vm tags for -zt */
	Z_VM_TAG_MASK   = 0xffff0000,
} zalloc_flags_t;

// `kfree` will freed via KHEAP_ANY(which means 0) type, but DO NOT `kfree` that data.kalloc.* kptr!
// THIS WILL panic from kfree_heap_confusion_panic(xnu-8019.41.5/osfmk/kern/kalloc.c:2003)
// use `kfree_ext(&KHEAP_DATA_BUFFERS, ...);` instead.

int main(int argc, char *argv[], char *envp[]) {

	offsets_init();

	if(init_tfp0() == KERN_SUCCESS) {
		printf("tfp0: 0x%" PRIx32 "\n", tfp0);

		int r = get_kbase(&kbase);
    	printf("get_kbase ret: %d, kbase: 0x%llx, kslide: 0x%llx\n", r, kbase, kslide);

		uint64_t our_task = task_self_addr();
		printf("our_task = 0x%llx\n", our_task);

		init_kexecute();

		//kexecute works
		uint64_t kret = kexecute(ksym(KSYMBOL_RET_300), 1, 0, 0, 0, 0, 0, 0);
		printf("kexecute KSYMBOL_RET_300 kret = 0x%llx\n", kret);

		// kcall8 works
		kret = kcall8(ksym(KSYMBOL_RET_300), 1, 0, 0, 0, 0, 0, 0, 0);
		printf("kcall8 KSYMBOL_RET_300 kret = 0x%llx\n", kret);

		// 1.
		// internally kext.kalloc.0x4000
		uint64_t kptr = kcall8(ksym(KSYMBOL_IOMalloc), 0x4000, 0, 0, 0, 0, 0, 0, 0);
		printf("kcall8 IOMalloc(0x4000) kptr = 0x%llx\n", kptr);

		kret = kcall8(ksym(KSYMBOL_IOFree), kptr, 0x4000, 0, 0, 0, 0, 0, 0);
		printf("kcall8 IOFree(kptr, 0x4000) kret = 0x%llx\n", kret);

		// 2.
		// kext.kalloc.0x4000
		kptr = kcall8(ksym(KSYMBOL_KALLOC_EXT), ksym(KHEAP_KEXT), 0x4000, Z_WAITOK, ksym(__ZZ17IOMalloc_internalE4site), 0, 0, 0, 0);
		printf("kcall8 kalloc_ext(KHEAP_KEXT, 0x4000, Z_WAITOK, &IOMalloc_internal::site) kptr = 0x%llx\n", kptr);

		kret = kcall8(ksym(KSYMBOL_KFREE), kptr, 0x4000, 0, 0, 0, 0, 0, 0);
		printf("kcall8 kfree(kptr, 0x4000) kret = 0x%llx\n", kret);

		// 3.
		// default.kalloc.0x4000
		kptr = kcall8(ksym(KSYMBOL_KALLOC_EXT), ksym(KHEAP_DEFAULT), 0x4000, Z_ZERO, ksym(necp_client_add_site), 0, 0, 0, 0);
		printf("kcall8 kalloc_ext(KHEAP_DEFAULT, 0x4000, Z_ZERO, &necp_client_add_site) kptr = 0x%llx\n", kptr);

		kret = kcall8(ksym(KSYMBOL_KFREE), kptr, 0x4000, 0, 0, 0, 0, 0, 0);
		printf("kcall8 kfree(kptr, 0x4000) kret = 0x%llx\n", kret);

		// 4. data.kalloc.0x4000
		kptr = kcall8(ksym(KSYMBOL_KALLOC_EXT), ksym(KHEAP_DATA_BUFFERS), 0x4000, Z_ZERO, ksym(pipespace_site), 0, 0, 0, 0);
		printf("kcall8 kalloc_ext(KHEAP_DATA_BUFFERS, 0x4000, Z_ZERO, &pipespace_site) kptr = 0x%llx\n", kptr);

		kret = kcall8(ksym(KSYMBOL_KFREE_EXT), ksym(KHEAP_DATA_BUFFERS), kptr, 0x4000, 0, 0, 0, 0, 0);
		printf("kcall8 kfree_ext(&KHEAP_DATA_BUFFERS, kptr, 0x4000) kret = 0x%llx\n", kret);

		kptr = kcall8(ksym(KSYMBOL_KALLOC_EXT), ksym(KHEAP_DATA_BUFFERS), 0x4000, Z_ZERO, ksym(pipespace_site), 0, 0, 0, 0);
		printf("kcall8 kalloc_ext(KHEAP_DATA_BUFFERS, 0x4000, Z_ZERO, &pipespace_site) kptr = 0x%llx\n", kptr);

		kret = kcall8(ksym(KSYMBOL_KFREE_EXT), ksym(KHEAP_DATA_BUFFERS), kptr, 0x4000, 0, 0, 0, 0, 0);
		printf("kcall8 kfree_ext(&KHEAP_DATA_BUFFERS, kptr, 0x4000) kret = 0x%llx\n", kret);

		// const uint64_t kalloc_cnt = 0x3a00;
		// const uint64_t kalloc_sz = 0x4000;
		// const uint64_t kalloc_sz2 = 0x3fcc;

		// uint64_t kheap_default_alloc[kalloc_cnt/8];
		// for (int i = 0; i < (kalloc_cnt/8); i++) {
		// 	kheap_default_alloc[i] = 0;
		// }
		// uint64_t kheap_data_buffers_alloc[kalloc_cnt/8];
		// for (int i = 0; i < (kalloc_cnt/8); i++) {
		// 	kheap_data_buffers_alloc[i] = 0;
		// }

		
		// const uint64_t ipc_kmsg_copyin_ool_ports_descriptor__site = 0xFFFFFFF009174358;
		// const uint64_t ipc_kmsg_alloc__site = 0xFFFFFFF009174358;
		// for(int i = 0; i < (kalloc_cnt/8); i++) {
		// 	kheap_default_alloc[i] = kcall8(ksym(KSYMBOL_KALLOC_EXT), ksym(KHEAP_DEFAULT), kalloc_sz, Z_WAITOK, (ipc_kmsg_copyin_ool_ports_descriptor__site + kslide), 0, 0, 0, 0);
		// 	printf("kallocated address (type: KHEAP_DEFAULT) = 0x%llx\n", kheap_default_alloc[i]);
		// }
		
		// for(int i = 0; i < (kalloc_cnt/8); i++) {
		// 	kheap_data_buffers_alloc[i] = kcall8(ksym(KSYMBOL_KALLOC_EXT), ksym(KHEAP_DATA_BUFFERS), kalloc_sz2, Z_WAITOK, (ipc_kmsg_alloc__site + kslide), 0, 0, 0, 0);
		// 	printf("kallocated address (type: KHEAP_DATA_BUFFERS) = 0x%llx\n", kheap_data_buffers_alloc[i]);
		// }
		// puts("Done\n");
		// getchar();
		
		// for(int i = 0; i < (kalloc_cnt/8); i++) {
		// 	kret = kcall8(ksym(KSYMBOL_KFREE), kheap_default_alloc[i], kalloc_sz, 0, 0, 0, 0, 0, 0);
		// }

		// for(int i = 0; i < (kalloc_cnt/8); i++) {
		// 	kret = kcall8(ksym(KSYMBOL_KFREE), kheap_data_buffers_alloc[i], kalloc_sz2, 0, 0, 0, 0, 0, 0);
		// }

		//panic
		// for(int i = 0; i < (0x4000/8); i++) {
		// 	kwrite64(kptr + i*8, 0);
		// }
		// khexdump(kptr, kalloc_sz+0x10);

		term_kexecute();

		mach_port_deallocate(mach_task_self(), tfp0);
	}
}
