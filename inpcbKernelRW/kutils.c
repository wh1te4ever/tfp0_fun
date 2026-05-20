#include "kextrw.h"
#include "offsets.h"
#include <unistd.h>
#include <assert.h>

uint64_t kread_smrptr(uint64_t va)
{
    uint64_t value = kextrw_kreadptr(va);

    uint64_t smr_base = 2;
    uint64_t t1sz_boot = 0x11;

    uint64_t bits = (smr_base << (62-t1sz_boot));
    
    if((value & bits) == 0) {
        return ((value & (0xFFFFFFFFFFFFC000LL & ~bits)) | bits);
    }
    return (value & 0xFFFFFFFFFFFFFFE0);
}

uint64_t proc_find(pid_t pid) {
    uint64_t proc = kextrw_kread64(ksym(KSYMBOL_KERNPROC));  

    while (1) {
        if(kextrw_kread32(proc + off_proc_p_pid) == pid) {
            return proc;
        }
        proc = kextrw_kread64(proc + off_proc_p_list_le_prev);
        if(!proc) {
            return -1;
        }
    }
    
    return 0;
}

uint64_t proc_task(uint64_t proc)
{
    uint64_t p_proc_ro = kextrw_kread64(proc + off_proc_p_proc_ro);
    uint64_t pr_task = kextrw_kread64(p_proc_ro + off_proc_ro_pr_task);
    return pr_task;
}

uint64_t task_self(void) {
    uint64_t selfProc = proc_find(getpid());
    uint64_t selfTask = proc_task(selfProc);
    return selfTask;
}

uint64_t ipc_entry_lookup(uint64_t space, mach_port_name_t name)
{
    uint64_t table = kread_smrptr(space + off_ipc_space_is_table);

	return (table + (sizeof_ipc_entry * (name >> 8)));
}

uint64_t task_get_ipc_port_table_entry(uint64_t task, mach_port_t port)
{
    uint64_t itk_space = kextrw_kreadptr(task + off_task_itk_space);
    uint64_t ret = ipc_entry_lookup(itk_space, port);
    return ret;
}

uint64_t task_get_ipc_port_object(uint64_t task, mach_port_t port)
{
    uint64_t ret = kextrw_kreadptr(task_get_ipc_port_table_entry(task, port) + off_ipc_entry_ie_object);
    return ret;
}

uint64_t task_get_ipc_port_kobject(uint64_t task, mach_port_t port)
{
    return kextrw_kreadptr(task_get_ipc_port_object(task, port) + off_ipc_port_ip_kobject);
}