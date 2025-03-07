#include <stdio.h>
#include <stdlib.h>
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <unistd.h>

#include "kdbg.h"
#include "krw.h"
#include "kutils.h"

#include "kernel_debug_me.h"

#include "iokit.h"

/* KDP is enabled and a client has connected and had a chance to set breakpoints.
 * Put the PoC code you want debugged in here.
 *
 * All the functions in iokit.h will use the kdp wrapper to ensure they are kernel-debuggable
 * note that these are just the raw MIG calls, not the IOKitLib wrappers.
 */
void kernel_debug_me() {
  mach_port_t master_port = MACH_PORT_NULL;
  host_get_io_master(mach_host_self(), &master_port);
  
  mach_port_t service = MACH_PORT_NULL;
  kern_return_t result = KERN_SUCCESS;
  
  char* matching = "<XML>NOT_REALLY_XML</XML>?<>!23?!@#123";
  mach_msg_type_number_t matchingCnt = strlen(matching) + 1;
  
  while(1) {
    printf("syscall starting\n");
    io_service_get_matching_service_ool(master_port, matching, matchingCnt, &result, &service);
    printf("syscall done\n");
  }

}
