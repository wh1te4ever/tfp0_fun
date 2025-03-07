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

// the offsets are unlikely to change between similar models and builds, but the symbol addresses will
// the offsets are required to get the kernel r/w but the symbols aren't

uint64_t *symbols = NULL;
int *offsets = NULL;
uint64_t kaslr_slide = 0;

/* iOS 11.1.2 */
uint64_t ksymbols_iphone_6s_15b202[] = {
    0xFFFFFFF00748D548, // KSYMBOL_OSARRAY_GET_META_CLASS,
    0xFFFFFFF00751C4D0, // KSYMBOL_IOUSERCLIENT_GET_META_CLASS
    0xFFFFFFF00751DC78, // KSYMBOL_IOUSERCLIENT_GET_TARGET_AND_TRAP_FOR_INDEX
    0xFFFFFFF0073A1054, // KSYMBOL_CSBLOB_GET_CD_HASH
    0xFFFFFFF0070B8088, // KSYMBOL_KALLOC_EXTERNAL
    0xFFFFFFF0070B80B8, // KSYMBOL_KFREE
    0xFFFFFFF0070B80B4, // KYSMBOL_RET
    0xFFFFFFF0074A7248, // KSYMBOL_OSSERIALIZER_SERIALIZE,
    0xFFFFFFF0075426C4, // KSYMBOL_KPRINTF
    0xFFFFFFF0074B21E0, // KSYMBOL_UUID_COPY
    0xFFFFFFF007566000, // KSYMBOL_CPU_DATA_ENTRIES         // 0x6000 in to the data segment
    0xFFFFFFF00708818C, // KSYMBOL_VALID_LINK_REGISTER      // look for reference to  FAR_EL1 (Fault Address Register (EL1))
    0xFFFFFFF007088164, // KSYMBOL_X21_JOP_GADGET           // look for references to FPCR (Floating-point Control Register)
    0xFFFFFFF007088434, // KSYMBOL_EXCEPTION_RETURN         // look for references to Set PSTATE.DAIF [--IF]
    0xFFFFFFF0070883E4, // KSYMBOL_THREAD_EXCEPTION_RETURN  // a bit before exception_return
    0xFFFFFFF007197AB0, // KSYMBOL_SET_MDSCR_EL1_GADGET     // look for references to MDSCR_EL1
    0xFFFFFFF0073EFB44, // KSYMBOL_WRITE_SYSCALL_ENTRYPOINT // look for references to enosys to find the syscall table (this is actually 1 instruction in to the entrypoint)
    0xFFFFFFF0071941D8, // KSYMBOL_EL1_HW_BP_INFINITE_LOOP  // look for xrefs to "ESR (0x%x) for instruction trapped" and find switch case 49
    0xFFFFFFF007194BBC, // KSYMBOL_SLEH_SYNC_EPILOG         // look for xrefs to "Unsupported Class %u event code."
    0xFFFFFFF00760A0A0, // KSYMBOL_KERNPROC
    0xFFFFFFF00719473C, // KSYMBOL_EL1_SW_STEP_INFINITE_LOOP
    0xFFFFFFF007197780, // KSYMBOL_ARM_DEBUG_SET
    0xFFFFFFF00701DA0C, // KSYMBOL_VERSION_STRING
    0xFFFFFFF0075A08C8, // KSYMBOL_KERNEL_UUID_STRING
};

uint64_t ksym(enum ksymbol sym)
{
    kaslr_slide = kbase - 0xFFFFFFF007004000;

    return symbols[sym] + kaslr_slide;
}

uint32_t off_ipc_port_ip_kobject = 0;
uint32_t off_ipc_port_ikmq_base = 0;
uint32_t off_ipc_port_msg_count = 0;
uint32_t off_ipc_port_to_io_bits = 0;
uint32_t off_ipc_port_ip_receiver = 0;

uint32_t off_ipc_space_is_table = 0;
uint32_t off_task_itk_space = 0;
uint32_t off_thread_bound_processor = 0; 
uint32_t off_thread_chosen_processor = 0;
uint32_t off_thread_context_data = 0;
uint32_t off_thread_kstackptr = 0;

uint32_t off_cpu_data_cpu_processor = 0;

uint32_t off_p_pid = 0;
uint32_t off_p_list_le_prev = 0;
uint32_t off_p_task = 0;

void offsets_init(void) {
    if (!(SYSTEM_VERSION_EQUAL_TO(@"11.1.2"))) {
        printf("[-] Only supported offset for iOS 11.1.2\n");
        exit(EXIT_FAILURE);
    }
    
    if (SYSTEM_VERSION_EQUAL_TO(@"11.1.2")) {
        printf("[i] offsets selected for iOS 11.1.2\n");

        off_ipc_port_ip_kobject = 0x68;
        off_ipc_port_ikmq_base = 0x40;
        off_ipc_port_msg_count = 0x50;
        off_ipc_port_to_io_bits = 0x0;
        off_ipc_port_ip_receiver = 0x60;

        off_ipc_space_is_table = 0x20;
        off_task_itk_space = 0x308;
        off_thread_bound_processor = 0x180; 
        off_thread_chosen_processor = 0x190;
        off_thread_context_data = 0x408;
        off_thread_kstackptr = 0x420;


        off_cpu_data_cpu_processor = 0x78;

        off_p_pid = 0x10;
        off_p_list_le_prev = 0x8;
        off_p_task = 0x18;
        

        symbols = ksymbols_iphone_6s_15b202;
    }
}
