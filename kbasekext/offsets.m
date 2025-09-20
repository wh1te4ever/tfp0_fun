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

uint64_t *symbols = NULL;
uint64_t kaslr_slide = 0;

uint64_t ksymbols_iphone_5s_14g60[] = {
    0xFFFFFFF0075A80C8, // KSYMBOL_KERNPROC
};

uint64_t ksym(enum ksymbol sym)
{
    kaslr_slide = kbase - 0xFFFFFFF007004000;

    return symbols[sym] + kaslr_slide;
}

uint32_t off_p_list_le_prev = 0;
uint32_t off_p_task = 0;
uint32_t off_p_pid = 0;
uint32_t off_p_flag = 0;

void offsets_init(void) {
    if (!(SYSTEM_VERSION_EQUAL_TO(@"10.3.3"))) {
        printf("[-] Only supported offset for iOS 10.3.3\n");
        exit(EXIT_FAILURE);
    }
    
    if (SYSTEM_VERSION_EQUAL_TO(@"10.3.3")) {
        printf("[i] offsets selected for iOS 10.3.3\n");

        off_p_list_le_prev = 0x8;
        off_p_task = 0x18;
        off_p_pid = 0x10;
        off_p_flag = 0x148;

        symbols = ksymbols_iphone_5s_14g60;
    }
}
