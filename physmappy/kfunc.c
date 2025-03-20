#include "kfunc.h"
#include "kcall8.h"
#include "krw.h"
#include "offsets.h"

uint64_t kfunc_kvtophys(uint64_t va) {
    uint64_t kret = kcall8(ksym(KSYMBOL_kvtophys), va, 0, 0, 0, 0, 0, 0, 0);
    return kret;
}

uint64_t kfunc_phystokv(uint64_t pa) {
    uint64_t kret = kcall8(ksym(KSYMBOL_phystokv), pa, 0, 0, 0, 0, 0, 0, 0);
    return kret;
}

kern_return_t kfunc_pmap_enter_options_addr(uint64_t pmap, uint64_t pa, uint64_t va)
{
	while (1) {
        uint64_t kret = kcall8(ksym(KSYMBOL_pmap_enter_options_addr), pmap, va, pa, VM_PROT_READ | VM_PROT_WRITE, 0, 0, 1, 1);
		if (kret != KERN_RESOURCE_SHORTAGE) {
			return kret;
		}
	}
}

uint64_t kfunc_pmap_remove_options(uint64_t pmap, uint64_t start, uint64_t end)
{
    uint64_t kret = kcall8(ksym(KSYMBOL_pmap_remove_options), pmap, start, end, 0x100, 0, 0, 0, 0);
	return kret;
}