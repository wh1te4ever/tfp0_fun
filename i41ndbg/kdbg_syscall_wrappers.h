#ifndef kdbg_syscall_wrappers_h
#define kdbg_syscall_wrappers_h

#include <mach/message.h>

mach_msg_return_t KDPWRAPPED_mach_msg(mach_msg_header_t *msg,
                                      mach_msg_option_t option,
                                      mach_msg_size_t send_size,
                                      mach_msg_size_t rcv_size,
                                      mach_port_name_t rcv_name,
                                      mach_msg_timeout_t timeout,
                                      mach_port_name_t notify);

#endif
