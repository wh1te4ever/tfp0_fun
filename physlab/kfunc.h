#include <stdint.h>
#include <mach/mach.h>

uint64_t kfunc_kvtophys(uint64_t va);
uint64_t kfunc_phystokv(uint64_t pa);
kern_return_t kfunc_pmap_enter_options_addr(uint64_t pmap, uint64_t pa, uint64_t va);
uint64_t kfunc_pmap_remove_options(uint64_t pmap, uint64_t start, uint64_t end);

uint32_t kfunc_physread32(uint64_t pa);
uint64_t kfunc_physread64(uint64_t pa);
void kfunc_physwrite64(uint64_t paddr, uint64_t value);
void kfunc_physwrite32(uint64_t paddr, uint32_t value);