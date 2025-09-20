#include <stdint.h>

enum ksymbol {
  KSYMBOL_KERNPROC
};

extern uint32_t off_p_list_le_prev;
extern uint32_t off_p_task;
extern uint32_t off_p_pid;
extern uint32_t off_p_flag;

uint64_t ksym(enum ksymbol sym);
void offsets_init(void);
