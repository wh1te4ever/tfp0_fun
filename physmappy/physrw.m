#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mach/mach.h>
#include <stdbool.h>
#include <errno.h>
#include "kutils.h"
#include "offsets.h"
#include "physrw.h"
#include "krw.h"
#include "pmap.h"
#include "physrw.h"
#include "translation.h"

void enumerate_pages(uint64_t start, size_t size, uint64_t pageSize, bool (^block)(uint64_t curStart, size_t curSize))
{
	uint64_t curStart = start;
	size_t sizeLeft = size;
	bool c = true;
	while (sizeLeft > 0 && c) {
		uint64_t pageOffset = curStart & (pageSize - 1);
		uint64_t readSize = min(sizeLeft, pageSize - pageOffset);
		c = block(curStart, readSize);
		curStart += readSize;
		sizeLeft -= readSize;
	}
}

int physreadbuf(uint64_t pa, void* output, size_t size)
{
	memset(output, 0, size);

	__block int pr = 0;
	enumerate_pages(pa, size, vm_real_kernel_page_size, ^bool(uint64_t curPhys, size_t curSize){
		uint64_t curKaddr = phystokv(curPhys);
		if (curKaddr == 0 && errno != 0) {
			pr = errno;
			return false;
		}
		pr = kreadbuf(curKaddr, &output[curPhys - pa], curSize);
		if (pr != 0) {
			return false;
		}
		return true;
	});
	return pr;
}

uint64_t physread64(uint64_t pa)
{
	uint64_t v;
	physreadbuf(pa, &v, sizeof(v));
	return v;
}

uint32_t physread32(uint64_t pa)
{
	uint32_t v;
	physreadbuf(pa, &v, sizeof(v));
	return v;
}

int physwritebuf(uint64_t pa, const void* input, size_t size)
{
	__block int pr = 0;
	enumerate_pages(pa, size, vm_real_kernel_page_size, ^bool(uint64_t curPhys, size_t curSize){
		uint64_t curKaddr = phystokv(curPhys);
		if (curKaddr == 0 && errno != 0) {
			pr = errno;
			return false;
		}
		pr = kwritebuf(curKaddr, &input[curPhys - pa], curSize);
		if (pr != 0) {
			return false;
		}
		return true;
	});
	return pr;
}

int physwrite8(uint64_t pa, uint8_t v)
{
	return physwritebuf(pa, &v, sizeof(v));
}


int physrw_handoff(pid_t pid)
{
	if (!pid) return -1;

	uint64_t proc = proc_of_pid(pid);
	if (!proc) return -2;

	int ret = 0;
	do {
		uint64_t task = kread64(proc + off_p_task);
		if (!task) { ret = -3; break; };

		uint64_t vmMap = kread64(task + off_task_map);
		if (!vmMap) { ret = -4; break; };

		uint64_t pmap = kread64(vmMap + off_vm_map_pmap);
		if (!pmap) { ret = -5; break; };

        uint64_t gPhysBase = kread64(ksym(KSYMBOL_gPhysBase));
        uint64_t gPhysSize = kread64(ksym(KSYMBOL_gPhysSize));
        // printf("PPLRW_USER_MAPPING_OFFSET = 0x%llx, gPhysBase = 0x%llx, gPhysSize = 0x%llx\n", PPLRW_USER_MAPPING_OFFSET, gPhysBase, gPhysSize);

		// Map the entire kernel physical address space into the userland process, starting at PPLRW_USER_MAPPING_OFFSET
		int mapInRet = pmap_map_in(pmap, gPhysBase+PPLRW_USER_MAPPING_OFFSET, gPhysBase, gPhysSize);
		if (mapInRet != 0) ret = -10 + mapInRet;
	} while (0);

	return ret;
}