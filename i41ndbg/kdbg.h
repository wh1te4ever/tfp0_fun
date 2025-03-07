#ifndef kdbg_h
#define kdbg_h

void test_kernel_bp(void);
uint64_t pin_current_thread(void);
void test_kdbg(void);
void test_fp(void);

int enable_breakpoint(uint64_t address);
int disable_breakpoint(uint64_t address);
void disable_all_breakpoints(void);
int breakpoint_is_enabled(uint64_t address);

// retval must point to a buffer of at least 16 bytes (two uint64_ts)
void run_syscall_under_kdp(uint32_t syscall_number, uint64_t* retval, uint32_t n_args, ...);

void prepare_current_thread_for_kdbg(void);
void update_hw_breakpoint_debugarea(void);

#endif
