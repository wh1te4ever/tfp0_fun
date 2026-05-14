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
uint64_t kgadgets_ipad_7_17h35[] = {
    0xFFFFFFF006019AFC, // KGADGET_POPULATE
    0xFFFFFFF006747A44, // KGADGET_PROLOGUE
    0xFFFFFFF006628A94, // KGADGET_MOV_X10_X1__BR_X2
    0xFFFFFFF00659DCAC, // KGADGET_MOV_X20_X3__BR_X2     
    0xFFFFFFF006556E14, // KGADGET_MOV_X13_X1__BR_X2     
    0xFFFFFFF006512534, // KGADGET_MOV_X11_X13__BR_X10   
    0xFFFFFFF00654C500, // KGADGET_MOV_X7_X1__BR_X8
    0xFFFFFFF00752BA84, // KGADGET_MOV_X0_X3__BR_X4
    0xFFFFFFF006B3D428, // KGADGET_MOV_X5_X8__BR_X10
    0xFFFFFFF0072EEECC, // KGADGET_MOV_X0_X20__BR_X11   
    0xFFFFFFF0074DF6F0, // KGADGET_ADD_X0_X0_0X40__RET
};

uint64_t kgad(enum kgadget gad)
{
    kaslr_slide = kbase - 0xFFFFFFF007004000;

    return gadgets[gad] + kaslr_slide;
}

uint64_t ksymbols_ipad_7_17h35[] = {
    0xFFFFFFF0077702A0, // KSYMBOL_KERNPROC
    0xFFFFFFF00612EB70, // KSYMBOL_RET_300
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
    if (!(SYSTEM_VERSION_EQUAL_TO(@"13.7"))) {
        printf("[-] Only supported offset for iOS 13.7\n");
        exit(EXIT_FAILURE);
    }
    
    if (SYSTEM_VERSION_EQUAL_TO(@"13.7")) {
        printf("[i] offsets selected for iOS 15.0\n");

        off_p_pid = 0x68; //v
        off_p_list_le_prev = 0x8;
        
        off_p_task = 0x10; //v
        
        off_task_itk_space = 0x320; //v
        off_ipc_space_is_table = 0x20;  //v
        off_ipc_space_is_task = 0x28; //v

        off_ipc_port_ip_kobject = 0x68; //v
        
        gadgets = kgadgets_ipad_7_17h35;
        symbols = ksymbols_ipad_7_17h35;
    }
}
