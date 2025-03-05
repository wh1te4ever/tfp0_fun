#include <mach/mach.h>
#include <stdint.h>
#include "iokit.h"

io_connect_t IOSurfaceRootUserClient;

vm_size_t get_kernel_page_size();
void IOSurfaceRoot_init(void);
void IOSurfaceRoot_exit(void);
uint32_t iosurface_s_create_surface_fast_path(void);
uint32_t iosurface_s_get_ycbcrmatrix(void);
void iosurface_s_set_indexed_timestamp(uint64_t v);

