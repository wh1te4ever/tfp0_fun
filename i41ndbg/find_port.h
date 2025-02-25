#include <stdint.h>
#include <mach/mach.h>

uint64_t find_port_via_kmem_read(mach_port_name_t port);
uint64_t find_port_address(mach_port_t port, int disposition);
