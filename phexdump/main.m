#include "helper/proc.h"
#include "helper/tfp0_find_port.h"
#include "helper/tfp0_krw.h"
#include "helper/offsets.h"
#include "helper/find_IOSurface.h"
#include "helper/find_pipe.h"
#include "helper/kexecute.h"
#include "helper/vram.h"
#include "helper/kcall8.h"
extern uint64_t gKernelSlide, gKernelBase;
extern task_t tfp0;


int main(int argc, char *argv[], char *envp[]) {
    tfp0_init();
    tfp0_get_kernel_base();
    offsets_init();
    printf("tfp0 = 0x%x\n", tfp0);
    printf("gKernelBase = 0x%llx, gKernelSlide = 0x%llx\n", gKernelBase, gKernelSlide);
    init_kexecute();

	uint64_t addr = 0;
	size_t   size = 0x100;

	if (argc >= 2) {
		addr = (uint64_t)strtoull(argv[1], NULL, 0);
	}
	if (argc >= 3) {
		size = (size_t)strtoull(argv[2], NULL, 0);
		phexdump(addr, size);
	}

    term_kexecute();
    tfp0_deinit();

    return 0;
}