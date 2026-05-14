#include <stdint.h>

enum kgadget {
  KGADGET_POPULATE,
  KGADGET_PROLOGUE,
  KGADGET_MOV_X10_X1__BR_X2,
  KGADGET_MOV_X20_X3__BR_X2,  
  KGADGET_MOV_X13_X1__BR_X2,  
  KGADGET_MOV_X11_X13__BR_X10,
  KGADGET_MOV_X7_X1__BR_X8,
  KGADGET_MOV_X0_X3__BR_X4,
  KGADGET_MOV_X5_X8__BR_X10,
  KGADGET_MOV_X0_X20__BR_X11, 
  KGADGET_ADD_X0_X0_0X40__RET,
};

enum ksymbol {
  KSYMBOL_KERNPROC,
  KSYMBOL_RET_300,
};

extern uint32_t off_p_pid;
extern uint32_t off_p_list_le_prev;
extern uint32_t off_p_task;
extern uint32_t off_task_itk_space;
extern uint32_t off_ipc_space_is_table;
extern uint32_t off_ipc_space_is_task;
extern uint32_t off_ipc_port_ip_kobject;

uint64_t kgad(enum kgadget sym);
uint64_t ksym(enum ksymbol sym);
void offsets_init(void);
