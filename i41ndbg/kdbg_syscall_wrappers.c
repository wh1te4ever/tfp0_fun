/*
 * This file contains wrappers around some syscalls to make them kernel-debuggable.
 * These wrappers are based on the XNU libsyscall implementations, libsyscall code is subject to the following licenses:
 */

/*
 * Copyright (c) 1999-2013 Apple Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */


#include <stdio.h>
#include <stdlib.h>
#include <mach/message.h>

#include "kdbg_syscall_wrappers.h"

#include "kdbg.h"

// wrappers around syscalls which call them with the kdp debugger enabled

uint32_t mach_msg_trap_number = 0xffffffe1; // -31

mach_msg_return_t KDPWRAPPED_mach_msg_trap(mach_msg_header_t *msg,
                                           mach_msg_option_t option,
                                           mach_msg_size_t send_size,
                                           mach_msg_size_t rcv_size,
                                           mach_port_name_t rcv_name,
                                           mach_msg_timeout_t timeout,
                                           mach_port_name_t notify) {
  uint64_t retval[2] = {0};
  run_syscall_under_kdp(mach_msg_trap_number, retval, 7,
                        msg,
                        option,
                        send_size,
                        rcv_size,
                        rcv_name,
                        timeout,
                        notify);
  return (mach_msg_return_t)retval[0];
}

// mach_msg isn't a syscall, it's a library function which wraps mach_msg_trap
// this implementation is based on XNU libsyscall

mach_msg_return_t KDPWRAPPED_mach_msg(mach_msg_header_t *msg,
                                      mach_msg_option_t option,
                                      mach_msg_size_t send_size,
                                      mach_msg_size_t rcv_size,
                                      mach_port_name_t rcv_name,
                                      mach_msg_timeout_t timeout,
                                      mach_port_name_t notify) {
  mach_msg_return_t mr;
  
  /*
   * Consider the following cases:
   *  1) Errors in pseudo-receive (eg, MACH_SEND_INTERRUPTED
   *  plus special bits).
   *  2) Use of MACH_SEND_INTERRUPT/MACH_RCV_INTERRUPT options.
   *  3) RPC calls with interruptions in one/both halves.
   *
   * We refrain from passing the option bits that we implement
   * to the kernel.  This prevents their presence from inhibiting
   * the kernel's fast paths (when it checks the option value).
   */
  
  mr = KDPWRAPPED_mach_msg_trap(msg, option &~ (MACH_SEND_INTERRUPT|MACH_RCV_INTERRUPT),
                     send_size, rcv_size, rcv_name,
                     timeout, notify);
  if (mr == MACH_MSG_SUCCESS) {
    return MACH_MSG_SUCCESS;
  }
  
  if ((option & MACH_SEND_INTERRUPT) == 0) {
    while (mr == MACH_SEND_INTERRUPTED) {
      mr = KDPWRAPPED_mach_msg_trap(msg,
                         option &~ (MACH_SEND_INTERRUPT|MACH_RCV_INTERRUPT),
                         send_size, rcv_size, rcv_name,
                         timeout, notify);
    }
  }
  
  if ((option & MACH_RCV_INTERRUPT) == 0) {
    while (mr == MACH_RCV_INTERRUPTED) {
      mr = KDPWRAPPED_mach_msg_trap(msg,
                         option &~ (MACH_SEND_INTERRUPT|MACH_RCV_INTERRUPT|MACH_SEND_MSG),
                         0, rcv_size, rcv_name,
                         timeout, notify);
    }
  }
  return mr;
}
