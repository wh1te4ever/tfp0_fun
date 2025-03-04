//
//  offsets.c
//  kfd
//
//  Created by Seo Hyun-gyu on 2023/07/29.
//

#include "offsets.h"
#include <stdint.h>
#include <UIKit/UIKit.h>
#include <Foundation/Foundation.h>

extern uint64_t kbase;

#define SYSTEM_VERSION_EQUAL_TO(v)                  ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] == NSOrderedSame)
#define SYSTEM_VERSION_GREATER_THAN(v)              ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] == NSOrderedDescending)
#define SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(v)  ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] != NSOrderedAscending)
#define SYSTEM_VERSION_LESS_THAN(v)                 ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] == NSOrderedAscending)
#define SYSTEM_VERSION_LESS_THAN_OR_EQUAL_TO(v)     ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] != NSOrderedDescending)

uint64_t *gadgets = NULL;
uint64_t *symbols = NULL;
uint64_t kaslr_slide = 0;

/* iOS 14.4.2 */
uint64_t kgadgets_iphone_8_18d70[] = {
    0xFFFFFFF0085DEE04, // KGADGET_POPULATE
    0xFFFFFFF008950124, // KGADGET_PROLOGUE
    0xFFFFFFF0087BD4F4, // KGADGET_MOV_X15_X2__BR_X3
    0xFFFFFFF0088191C0, // KGADGET_MOV_X10_X0__BR_X2
    0xFFFFFFF00874C65C, // KGADGET_MOV_X12_X0__BR_x2
    0xFFFFFFF0087E8564, // KGADGET_MOV_X16_X15__BR_X12
    0xFFFFFFF008801D90, // KGADGET_MOV_X7_X16__BR_X10
    0xFFFFFFF00877A80C, // KGADGET_MOV_X10_X0__BR_X12
    0xFFFFFFF008754D50, // KGADGET_MOV_X13_X2__BR_X12
    0xFFFFFFF00881B5CC, // KGADGET_MOV_X4_X13__BR_X15
    0xFFFFFFF007A9EEBC, // KGADGET_ADD_X0_X0_0X40__RET
};

uint64_t kgad(enum kgadget gad)
{
    kaslr_slide = kbase - 0xFFFFFFF007004000;

    return gadgets[gad] + kaslr_slide;
}

uint64_t ksymbols_iphone_8_18d70[] = {
    0xFFFFFFF00773C468, // KSYMBOL_KERNPROC
    0xFFFFFFF007D52960, // KSYMBOL_RET_300
    0xFFFFFFF007A6182C, // KSYMBOL_KALLOC_EXTERNAL
    0xFFFFFFF007A618E8, // KSYMBOL_KFREE
    0xFFFFFFF009166C28, // KSYMBOL_PANIC
    0xFFFFFFF0077161C8, // KSYMBOL_gPhysBase
    0xFFFFFFF0077162B8, // KSYMBOL_gPhysSize
    0xFFFFFFF0077161D0, // KSYMBOL_gVirtBase
    0xFFFFFFF007738160, // KSYMBOL_ptov_table
    0xFFFFFFF007B70B30, // KSYMBOL_phystokv
    0xFFFFFFF007B658D4, // KSYMBOL_kvtophys
    0xFFFFFFF007715CF0, // KSYMBOL_cpu_ttep
    0xFFFFFFF007B5EEBC, // KSYMBOL_pmap_enter_options_addr
    0xFFFFFFF007B63A70, // KSYMBOL_pmap_remove_options
};

uint64_t ksym(enum ksymbol sym)
{
    kaslr_slide = kbase - 0xFFFFFFF007004000;

    return symbols[sym] + kaslr_slide;
}

uint32_t off_ipc_port_ip_kobject = 0;
uint32_t off_task_itk_space = 0;
uint32_t off_ipc_space_is_table = 0;
uint32_t off_p_pid = 0;
uint32_t off_p_pfd = 0;
uint32_t off_p_list_le_prev = 0;
uint32_t off_p_task = 0;
uint32_t off_fg_data = 0;
uint32_t off_fp_fglob = 0;
uint32_t off_pb_buffer = 0;
uint32_t off_task_map = 0;
uint32_t off_vm_map_pmap = 0;
uint32_t off_pmap_ttep = 0;
uint32_t off_pmap_type = 0;

void offsets_init(void) {
    if (!(SYSTEM_VERSION_EQUAL_TO(@"14.4.2"))) {
        printf("[-] Only supported offset for iOS 14.4.2\n");
        exit(EXIT_FAILURE);
    }
    
    if (SYSTEM_VERSION_EQUAL_TO(@"14.4.2")) {
        printf("[i] offsets selected for iOS 14.4.2\n");

        off_ipc_port_ip_kobject = 0x68;
        off_task_itk_space = 0x330;
        off_ipc_space_is_table = 0x20;

        off_p_pid = 0x68;
        off_p_pfd = 0xf8;

        off_p_list_le_prev = 0x8;

        off_p_task = 0x10;

        off_fg_data = 0x38;
        off_fp_fglob = 0x10;

        //https://github.com/apple-oss-distributions/xnu/blob/rel/xnu-8019/bsd/sys/pipe.h#L102
        // (lldb) p/x offsetof(struct pipebuf, buffer)
        // (unsigned long) 0x0000000000000010
        off_pb_buffer = 0x10;

        //https://github.com/apple-oss-distributions/xnu/blob/xnu-7195.81.3/osfmk/kern/task.h
        off_task_map = 0x28;

        //https://github.com/apple-oss-distributions/xnu/blob/xnu-7195.81.3/osfmk/vm/vm_map.h#L466
        off_vm_map_pmap = 0x48;

        //https://github.com/apple-oss-distributions/xnu/blob/xnu-7195.81.3/osfmk/arm/pmap.h#L352
        off_pmap_ttep = 0x8;

        //Xref string: pmap_trim_internal
        off_pmap_type = 0xE2;   //A11 = 0xE2, A9 = 0xE1

        gadgets = kgadgets_iphone_8_18d70;
        symbols = ksymbols_iphone_8_18d70;
    }
}
