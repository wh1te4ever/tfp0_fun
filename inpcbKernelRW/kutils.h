#include <stdint.h>
#include <unistd.h>
#include <mach/mach.h>

uint64_t proc_find(pid_t pid);
uint64_t proc_task(uint64_t proc);
uint64_t task_self(void);
uint64_t ipc_entry_lookup(uint64_t space, mach_port_name_t name);
uint64_t task_get_ipc_port_table_entry(uint64_t task, mach_port_t port);
uint64_t task_get_ipc_port_kobject(uint64_t task, mach_port_t port);