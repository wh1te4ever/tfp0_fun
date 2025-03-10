#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mach/mach.h>
#include <mach-o/dyld.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include "kutils.h"
#include "offsets.h"
#include "physrw.h"
#include "krw.h"
#include "pmap.h"
#include "physrw.h"
#include "translation.h"
#include "pte.h"
#include "pvh.h"
#include "kfunc.h"

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

uint16_t physread16(uint64_t pa)
{
	uint16_t v;
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

int physwrite16(uint64_t pa, uint16_t v)
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

//phys r/w using PPLRW_USER_MAPPING_OFFSET
void *physrw_phystouaddr(uint64_t pa)
{
	errno = 0;

	uint64_t physBase = kread64(ksym(KSYMBOL_gPhysBase)), physSize = kread64(ksym(KSYMBOL_gPhysSize));
	bool doBoundaryCheck = (physBase != 0 && physSize != 0);
	if (doBoundaryCheck) {
		if (pa < physBase || pa >= (physBase + physSize)) {
			errno = 1030;
			return 0;
		}
	}

	return (void *)(pa + PPLRW_USER_MAPPING_OFFSET);
}

int physrw_physreadbuf(uint64_t pa, void* output, size_t size)
{
	void *uaddr = physrw_phystouaddr(pa);
	if (!uaddr && errno != 0) {
		memset(output, 0x0, size);
		return errno;
	}

	asm volatile("dmb sy");
	memcpy(output, uaddr, size);
	return 0;
}

int physrw_physwritebuf(uint64_t pa, const void* input, size_t size)
{
	void *uaddr = physrw_phystouaddr(pa);
	if (!uaddr && errno != 0) {
		return errno;
	}

	memcpy(uaddr, input, size);
	asm volatile("dmb sy");
	return 0;
}

int kreadbuf_phys(uint64_t kaddr, void* output, size_t size)
{
	memset(output, 0, size);

	__block int pr = 0;
	enumerate_pages(kaddr, size, vm_real_kernel_page_size, ^bool(uint64_t curKaddr, size_t curSize){
		uint64_t curPhys = kvtophys(curKaddr);
		if (curPhys == 0 && errno != 0) {
			pr = errno;
			return false;
		}
		pr = physrw_physreadbuf(curPhys, &output[curKaddr - kaddr], curSize);
		if (pr != 0) {
			return false;
		}
		return true;
	});
	return pr;
}

int kwritebuf_phys(uint64_t kaddr, const void* input, size_t size)
{
	__block int pr = 0;
	enumerate_pages(kaddr, size, vm_real_kernel_page_size, ^bool(uint64_t curKaddr, size_t curSize){
		uint64_t curPhys = kvtophys(curKaddr);
		if (curPhys == 0 && errno != 0) {
			pr = errno;
			return false;
		}
		pr = physrw_physwritebuf(curPhys, &input[curKaddr - kaddr], curSize);
		if (pr != 0) {
			return false;
		}
		return true;
	});
	return pr;
}

uint32_t kread32_phys(uint64_t where) {
    uint32_t out;
    kreadbuf_phys(where, &out, sizeof(uint32_t));
    return out;
}

uint64_t kread64_phys(uint64_t where) {
    uint64_t out;
    kreadbuf_phys(where, &out, sizeof(uint64_t));
    return out;
}

void kwrite32_phys(uint64_t where, uint32_t what) {
    uint32_t _what = what;
    kwritebuf_phys(where, &_what, sizeof(uint32_t));
}

void kwrite64_phys(uint64_t where, uint64_t what) {
    uint64_t _what = what;
    kwritebuf_phys(where, &_what, sizeof(uint64_t));
}


int physrw_lab(void)
{
	uint64_t proc = proc_by_name("MobileTimer");
	printf("timer_proc = 0x%llx\n", proc);
	if (!proc) return -2;

	int ret = 0;

	uint64_t task = kread64(proc + off_p_task);
	if (!task) { ret = -3; };

	uint64_t vmMap = kread64(task + off_task_map);
	if (!vmMap) { ret = -4; };

	uint64_t pmap = kread64(vmMap + off_vm_map_pmap);
	if (!pmap) { ret = -5; };

	uint64_t ttep = kread64(pmap + off_pmap_ttep);
    printf("[*] ttep = 0x%llx\n", ttep);

	uint64_t uaInstr = 0x000000010004EFB8;

	uint64_t leafLevel = PMAP_TT_L3_LEVEL;
	uint64_t paInstr = vtophys_lvl(ttep, uaInstr, &leafLevel, NULL);
	printf("[+] uaInstr = 0x%llx -> paInstr = 0x%llx\n", uaInstr, paInstr);

	// uint64_t phys_kbase = kfunc_kvtophys(kbase);
	uint64_t val = kfunc_physread64(paInstr);
	// printf("val = 0x%llx\n", val);

	int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

	kfunc_physwrite32(paInstr, 0x14000000);
	printf("[!] Timer is now playing with 1337, 4141~! Have fun!\n");
	printf("[!] Enter 'q' to exit loop.\n");

	while (1) {
		kfunc_physwrite32(paInstr+16, 0x9e670040);
		kfunc_physwrite32(paInstr+12, 0xf2f81122);
		kfunc_physwrite32(paInstr+8, 0xf2d12662);
		kfunc_physwrite32(paInstr+4, 0xf2a66662);
		kfunc_physwrite32(paInstr, 0xd2866662);
		
		usleep(50000);

		kfunc_physwrite32(paInstr, 0xd29999a2);
		kfunc_physwrite32(paInstr+4, 0xf2b99982);
		kfunc_physwrite32(paInstr+8, 0xf2d15982);
		kfunc_physwrite32(paInstr+12, 0xf2f81462);
		kfunc_physwrite32(paInstr+16, 0x9e670040);
		usleep(50000);

        if (getchar() == 'q') {
            break;
        }
	}

	//restore
	kfunc_physwrite32(paInstr, 0x9838a954);
	kfunc_physwrite32(paInstr+4, 0xf8746800);
	kfunc_physwrite32(paInstr+8, 0xd503201f);
	kfunc_physwrite32(paInstr+12, 0x5837cda1);
	kfunc_physwrite32(paInstr+16, 0x9400badb);

	fcntl(STDIN_FILENO, F_SETFL, flags);

	// 626686D2
	// 6266A6F2
	// 6226D1F2
	// 2211F8F2
	// 4000679E
	// 06000014

	// A29999D2
	// 8299B9F2
	// 8259D1F2
	// 6214F8F2
	// 4000679E

	// __text:000000010004EFB8 54 A9 38 98                                                     LDRSW           X20, =0x10 ; NSDate *_startTime;
	// __text:000000010004EFBC 00 68 74 F8                                                     LDR             X0, [X0,X20] ; id
	// __text:000000010004EFC0 1F 20 03 D5                                                     NOP
	// __text:000000010004EFC4 A1 CD 37 58                                                     LDR             X1, =sel_timeIntervalSinceNow ; "timeIntervalSinceNow"
	// __text:000000010004EFC8 DB BA 00 94                                                     BL              _objc_msgSend ; -[NSDate timeIntervalSinceNow] ...
	

	return ret;
}