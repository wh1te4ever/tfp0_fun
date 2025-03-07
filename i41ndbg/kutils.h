#include <stdint.h>
#include <unistd.h>
#include <mach/mach.h>

uint64_t proc_of_pid(pid_t pid);
uint64_t task_self_addr(void);
uint64_t ipc_space_kernel(void);
uint64_t current_thread();

uint64_t thread_get_debug_area(mach_port_t thread_port);