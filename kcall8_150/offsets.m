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

/* iOS 15.0 */
uint64_t kgadgets_ipad_7_19a346[] = {
    0xFFFFFFF005B1F640, // KGADGET_POPULATE
    0xFFFFFFF006442458, // KGADGET_PROLOGUE
    0xFFFFFFF00625587C, // KGADGET_MOV_X5_X6__BR_X15
    0xFFFFFFF0062BB564, // KGADGET_MOV_X6_X9__BR_X11
    0xFFFFFFF0061D6514, // KGADGET_MOV_X7_X14__BR_X15
    0xFFFFFFF00624452C, // KGADGET_MOV_X9_X1__BR_X10
    0xFFFFFFF00620D7E8, // KGADGET_MOV_X15_X2__BR_X3
    0xFFFFFFF0061B933C, // KGADGET_MOV_X15_X1__BR_X2
    0xFFFFFFF0061FCB98, // KGADGET_MOV_X14_X3__BR_X4
    0xFFFFFFF0062E20E0, // KGADGET_MOV_X11_X4__BR_X2
    0xFFFFFFF0062E1264, // KGADGET_MOV_X10_X1__BR_X3
    0xFFFFFFF0059ACAC0, // KGADGET_ADD_X0_X0_0X40__RET
};

uint64_t kgad(enum kgadget gad)
{
    kaslr_slide = kbase - 0xFFFFFFF007004000;

    return gadgets[gad] + kaslr_slide;
}

uint64_t ksymbols_ipad_7_19a346[] = {
    0xFFFFFFF0071362B0, // KSYMBOL_KERNPROC
    0xFFFFFFF005B54B60, // KSYMBOL_RET_300
    0xFFFFFFF0071C6074, // KSYMBOL_KFREE
    0xFFFFFFF007807C68, // KSYMBOL_PANIC
    0xFFFFFFF0071C59E8, // KSYMBOL_KALLOC_EXT
    0xFFFFFFF0070FB350, // KHEAP_DATA_BUFFERS
    0xFFFFFFF0070FAD58, // KHEAP_DEFAULT
    0xFFFFFFF0070FB828, // KHEAP_KEXT
    0xFFFFFFF00782B500, // necp_client_add_site
    0xFFFFFFF007729F10, // KSYMBOL_IOMalloc
    0xFFFFFFF007729FCC, // KSYMBOL_IOFree
    0xFFFFFFF007841C60, // __ZZ17IOMalloc_internalE4site
    0xFFFFFFF00783A340, // pipespace_site
    0xFFFFFFF0071C60A4, // KSYMBOL_kfree_ext
};

uint64_t ksym(enum ksymbol sym)
{
    kaslr_slide = kbase - 0xFFFFFFF007004000;

    return symbols[sym] + kaslr_slide;
}

uint32_t off_p_pid = 0;
uint32_t off_p_list_le_prev = 0;
uint32_t off_p_task = 0;
uint32_t off_task_itk_space = 0;
uint32_t off_ipc_space_is_table = 0;
uint32_t off_ipc_space_is_task = 0;
uint32_t off_ipc_port_ip_receiver = 0;
uint32_t off_ipc_port_ip_kobject = 0;
uint32_t off_ipc_port_ikmq_base = 0;
uint32_t off_ipc_kmsg_ikm_header = 0;
uint32_t off_ipc_kmsg_ikm_data = 0;
uint32_t off_mach_msg_header_t_msgh_remote_port = 0;


void offsets_init(void) {
    if (!(SYSTEM_VERSION_EQUAL_TO(@"15.0"))) {
        printf("[-] Only supported offset for iOS 15.0\n");
        exit(EXIT_FAILURE);
    }
    
    if (SYSTEM_VERSION_EQUAL_TO(@"15.0")) {
        printf("[i] offsets selected for iOS 15.0\n");

        off_p_pid = 0x68;
        off_p_list_le_prev = 0x8;
        
        off_p_task = 0x10;
        
        off_task_itk_space = 0x330;
        off_ipc_space_is_table = 0x20;
        off_ipc_space_is_task = 0x30;
        
        off_ipc_port_ip_receiver = 0x50;
        off_ipc_port_ip_kobject = 0x58;
        off_ipc_port_ikmq_base = 0x30;
        
        off_ipc_kmsg_ikm_header = 0x18;
        off_ipc_kmsg_ikm_data = 0x10;
        
        off_mach_msg_header_t_msgh_remote_port = 0x8;
        
        gadgets = kgadgets_ipad_7_19a346;
        symbols = ksymbols_ipad_7_19a346;
    }
}
