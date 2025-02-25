#include "kutils.h"
#include "krw.h"
#include "offsets.h"
#include "find_port.h"

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>

// uint64_t cached_task_self_addr = 0;
// uint64_t task_self_addr() {
//     if (cached_task_self_addr == 0) {
//       cached_task_self_addr = find_port_address(mach_task_self(), MACH_MSG_TYPE_COPY_SEND);
//       printf("task self: 0x%llx\n", cached_task_self_addr);
//     }
//     return cached_task_self_addr;
// }

uint64_t proc_of_pid(pid_t pid) {
    uint64_t proc = kread64(ksym(KSYMBOL_KERNPROC));
    
    while (1) {
        if(kread32(proc + off_p_pid) == pid) {
            return proc;
        }
        proc = kread64(proc + off_p_list_le_prev);
        if(!proc) {
            return -1;
        }
    }
    
    return 0;
}

uint64_t task_self_addr() {
    uint64_t proc = proc_of_pid(getpid());
    uint64_t task = kread64(proc + off_p_task);
    return task;
}


uint64_t ipc_space_kernel() {
    return kread64(task_self_addr() + off_ipc_port_ip_receiver);
}

uint64_t current_thread() {
    uint64_t thread_port = find_port_address(mach_thread_self(), MACH_MSG_TYPE_COPY_SEND);
    return kread64(thread_port + off_ipc_port_ip_kobject);
}