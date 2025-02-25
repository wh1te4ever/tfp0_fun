#include <stdint.h>

enum ksymbol {
    KSYMBOL_OSARRAY_GET_META_CLASS,
    KSYMBOL_IOUSERCLIENT_GET_META_CLASS,
    KSYMBOL_IOUSERCLIENT_GET_TARGET_AND_TRAP_FOR_INDEX,
    KSYMBOL_CSBLOB_GET_CD_HASH,
    KSYMBOL_KALLOC_EXTERNAL,
    KSYMBOL_KFREE,
    KSYMBOL_RET,
    KSYMBOL_OSSERIALIZER_SERIALIZE,
    KSYMBOL_KPRINTF,
    KSYMBOL_UUID_COPY,
    KSYMBOL_CPU_DATA_ENTRIES,
    KSYMBOL_VALID_LINK_REGISTER,
    KSYMBOL_X21_JOP_GADGET,
    KSYMBOL_EXCEPTION_RETURN,
    KSYMBOL_THREAD_EXCEPTION_RETURN,
    KSYMBOL_SET_MDSCR_EL1_GADGET,
    KSYMBOL_WRITE_SYSCALL_ENTRYPOINT,
    KSYMBOL_EL1_HW_BP_INFINITE_LOOP,
    KSYMBOL_SLEH_SYNC_EPILOG,
    KSYMBOL_KERNPROC
  };

extern uint32_t off_ipc_port_ip_kobject;
extern uint32_t off_ipc_port_ikmq_base;
extern uint32_t off_ipc_port_msg_count;
extern uint32_t off_ipc_port_to_io_bits;
extern uint32_t off_ipc_port_ip_receiver;


extern uint32_t off_ipc_space_is_table;
extern uint32_t off_task_itk_space;

extern uint32_t off_thread_bound_processor;
extern uint32_t off_thread_chosen_processor;
extern uint32_t off_thread_context_data;
extern uint32_t off_thread_kstackptr;

extern uint32_t off_cpu_data_cpu_processor;

extern uint32_t off_p_pid;
extern uint32_t off_p_list_le_prev;
extern uint32_t off_p_task;

uint64_t ksym(enum ksymbol sym);
void offsets_init(void);