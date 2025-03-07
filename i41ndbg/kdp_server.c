#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

// the thread state structure used by KDP
#include "arm_thread_state64.h"

// the thread state structure as it appears on the kernel stack in exception frames
#include "arm64_state.h"


#include "kutils.h"
#include "offsets.h"
#include "krw.h"
#include "kdbg.h"

extern uint64_t kbase;

#define SERVER_MODE_TCP 1


// forward declarations of the ARM64 REIL stuff
typedef uint8_t (*read_byte_handler)(uint64_t address);
typedef void(*write_byte_handler)(uint64_t address, uint8_t value);

int arm64_emulate(struct arm_thread_state64* native_state, read_byte_handler read_func, write_byte_handler write_func);

/*
 *  lldb -> device:
 *    REQUEST_KDP_REATTACH
 *
 *  (device treats this as a KDP_DISCONNECT request. The reply it sends has the same structure as
 *   REPLY_KDP_DISCONNECT, but the command is still REPLY_KDP_REATTACH)
 *  device -> lldb:
 *    REPLY_KDP_REATTACH
 *
 */

// single, non-fragmented IPv4 wrapped UDP packet
#define KDP_MAX_REQUEST_SIZE 1472

// largest size for inline variable-sized buffers
#define KDP_MAX_INLINE_DATA_SIZE 1024

#define KDP_SUCCESS 0
#define KDP_ERROR_BAD_BYTE_COUNT 2
#define KDP_ERROR_UNKNOWN_STATE_FLAVOR 3
#define KDP_ERROR_TOO_MANY_BREAKPOINTS 100
#define KDP_ERROR_NO_SUCH_BREAKPOINT 101

void error_fatal(const char* message) {
  fprintf(stderr, "%s\n", message);
  exit(EXIT_FAILURE);
}

void error_warn(const char* message) {
  fprintf(stderr, "%s\n", message);
}

// the lldb continue and single_step commands don't work properly
int do_continue = 0;
int do_single_step = 0;

struct kdp_packet_header {    // size = 8 bytes
  uint8_t  command:7;
  uint8_t  is_reply:1;
  uint8_t  sequence_number;
  uint16_t total_length;
  uint32_t session_key;
} __attribute__((packed));

/* these packets are used in the handshake */

#define COMMAND_KDP_CONNECT 0
struct kdp_packet_connect_request {
  struct kdp_packet_header header;
  uint16_t reply_port;        // these are little-endian in the packet buffer, not network
  uint16_t exception_port;    //
  char     greeting[KDP_MAX_INLINE_DATA_SIZE];
} __attribute__((packed));

struct kdp_packet_connect_reply {
  struct kdp_packet_header header;
  uint32_t error;
} __attribute__((packed));


#define COMMAND_KDP_REATTACH 18
struct kdp_packet_reattach_request {
  struct kdp_packet_header header;
  uint16_t reply_port;
} __attribute__((packed));

struct kdp_packet_reattach_reply {
  struct kdp_packet_header header;
} __attribute__((packed));


struct sockaddr_in sa_tcp_client_addr = {0};

/*
 * create and bind a UDP socket
 */
int create_server_socket(uint16_t port) {
#ifdef SERVER_MODE_TCP
  int fd = socket(PF_INET, SOCK_STREAM, 0);
#else
  int fd = socket(PF_INET, SOCK_DGRAM, 0);
#endif
  
  if (fd == -1) {
    perror("unable to create socket for server");
    exit(EXIT_FAILURE);
  }
  
  struct sockaddr_in sa = {0};
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  inet_pton(AF_INET, "0.0.0.0", &(sa.sin_addr));
  
  int err = bind(fd, (struct sockaddr*)&sa, sizeof(sa));
  if (err == -1) {
    perror("unable to bind socket for server");
    exit(EXIT_FAILURE);
  }
  
  printf("bound server socket\n");
  
  
#ifdef SERVER_MODE_TCP
  err = listen(fd, 1);
  if (err != 0) {
    perror("unable to listen on server socket");
    exit(EXIT_FAILURE);
  }
  
  socklen_t sa_len = sizeof(sa_tcp_client_addr);
  int client_sock = accept(fd, (struct sockaddr*)&sa_tcp_client_addr, &sa_len);
  if (client_sock == -1) {
    perror("unable to accept new connection");
    exit(EXIT_FAILURE);
  }
  
  return client_sock;
  
#else
  return fd;
#endif
}

uint16_t kdp_well_known_port = 41139;

void read_n(int sock, void* buffer, size_t len) {
  uint8_t* bytes = buffer;
  size_t offset = 0;
  while (len > 0) {
    ssize_t n_read = recv(sock, bytes+offset, len, 0);
    if (n_read == -1) {
      perror("read_n got an error\n");
      exit(EXIT_FAILURE);
    }
    len -= n_read;
    offset += n_read;
  }
}

ssize_t receive_packet(int socket_fd, void* buffer, size_t buffer_length, struct in_addr* remote_address) {
#ifdef SERVER_MODE_TCP
  uint32_t payload_len = 0;
  read_n(socket_fd, &payload_len, 4);
  printf("payload len: %d\n", payload_len);
  
  if (payload_len > buffer_length) {
    printf("received oversized payload (payload length according to packet header: %d, buffer_length: %zd\n", payload_len, buffer_length);
    exit(EXIT_FAILURE);
  }
  
  read_n(socket_fd, buffer, payload_len);
  
  ssize_t received_count = payload_len;
#if 0
  ssize_t recv_len = recv(socket_fd, &payload_len, sizeof(payload_len), MSG_WAITALL);
  if (recv_len == -1) {
    perror("error receiving payload length\n");
    exit(EXIT_FAILURE);
  }
  if (recv_len != sizeof(uint32_t)) {
    printf("error receiveing payload length, expected 4 bytes, got %zd bytes\n", recv_len);
  }
  
  printf("read payload length from packet: %d\n", payload_len);
  
  if (payload_len > buffer_length) {
    printf("received oversized payload (payload length according to packet header: %d, buffer_length: %zd\n", payload_len, buffer_length);
    exit(EXIT_FAILURE);
  }
  
  // receive the payload
  ssize_t received_count = recv(socket_fd, buffer, payload_len, MSG_WAITALL);
  
  if (received_count == -1) {
    perror("error receiving wrapped payload");
    exit(EXIT_FAILURE);
  }
  
  printf("total read payload: %zd\n", received_count);
#endif
  *remote_address = sa_tcp_client_addr.sin_addr;
#else
  struct sockaddr_storage source_address = {0};
  socklen_t source_address_length = sizeof(source_address);
  
  ssize_t received_count = recvfrom(socket_fd,
                                    buffer,
                                    buffer_length,
                                    0,
                                    (struct sockaddr*)&source_address,
                                    &source_address_length);
  
  *remote_address = ((struct sockaddr_in*)&source_address)->sin_addr;
#endif
  
  
  if (received_count == -1) {
    perror("failed to recvfrom on socket");
    exit(EXIT_FAILURE);
  }
  
  // is the packet long enough to contain the kdp header?
  if (received_count < sizeof(struct kdp_packet_header)) {
    error_warn("received packet too short");
    return -1;
  }
  
  struct kdp_packet_header* header = (struct kdp_packet_header*)buffer;
  
  // does the length in the packet header match the received length?
  if (received_count != header->total_length) {
    error_warn("received packet has inconsistent length");
    // continue here because lldb does send packets with inconsistent lengths...
  }
  
  printf("received valid packet! length: %zd\n", received_count);
  
  return received_count;
}

ssize_t send_packet(int socket_fd, void* buffer, size_t buffer_length, struct in_addr destination_address, uint16_t port) {
#ifdef SERVER_MODE_TCP
  uint32_t port_u32 = port;
  uint32_t len_u32 = (uint32_t)buffer_length;
  
  ssize_t sent_count = send(socket_fd, &port_u32, sizeof(uint32_t), 0);
  if (sent_count != sizeof(uint32_t)) {
    printf("unable to fully send reply port number (sent_count %zd)\n", sent_count);
    exit(EXIT_FAILURE);
  }
  
  sent_count = send(socket_fd, &len_u32, sizeof(uint32_t), 0);
  if (sent_count != sizeof(uint32_t)) {
    printf("unable to fully send reply length (sent_count %zd)\n", sent_count);
    exit(EXIT_FAILURE);
  }
  
  sent_count = send(socket_fd, buffer, buffer_length, 0);
  if (sent_count != buffer_length) {
    printf("unable to fully send buffer (sent_count %zd)\n", sent_count);
    exit(EXIT_FAILURE);
  }
  
  return sent_count;
#else
  struct sockaddr_in sa = {0};
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  sa.sin_addr = destination_address;
  
  printf("about to try to send %zd bytes\n", buffer_length);
  
  ssize_t sent_count = sendto(socket_fd,
                              buffer,
                              buffer_length,
                              0,
                              (struct sockaddr*)&sa,
                              (socklen_t)sizeof(sa));
  
  printf("sent_count: %zd\n", sent_count);
  
  return sent_count;
#endif
}

ssize_t expect_packet(uint8_t command, int socket_fd, void* buffer, size_t min_size, size_t max_size, struct in_addr* remote_address) {
  ssize_t received_count = receive_packet(socket_fd, buffer, max_size, remote_address);
  
  if (received_count <= 0) {
    error_warn("expect_packet recevied an invalid packet");
    return received_count;
  }
  
  // receive_packet verifies that the received packet contains at least a header
  struct kdp_packet_header* header = (struct kdp_packet_header*)buffer;
  
  // does the command match the expected one?
  if (header->command != command) {
    error_warn("expect_packet received a packet with the wrong command");
    printf("expected %d, got %d\n", command, header->command);
    return -1;
  }
  
  // is the packet big enough
  if (header->total_length < min_size) {
    error_warn("expect_packet received a packet which was too small");
    return -1;
  }
  
  return received_count;
}

void log_request(const char* command) {
  printf("REQUEST: %s\n", command);
}

/*** command handlers ***/

#define COMMAND_KDP_VERSION 3
struct kdp_packet_version_request {
  struct kdp_packet_header header;
} __attribute__((packed));

struct kdp_packet_version_reply {
  struct kdp_packet_header header;
  uint32_t version;
  uint32_t features;
  uint8_t padding[8];
} __attribute__((packed));

int command_kdp_version(struct kdp_packet_header* request, struct kdp_packet_header* reply) {
  log_request("KDP_VERSION");
  struct kdp_packet_version_reply* version_reply = (struct kdp_packet_version_reply*)reply;
  
  version_reply->header.total_length = sizeof(struct kdp_packet_version_reply);
  
  // version 12 supports physical memory r/w
  version_reply->version = 12;
  
  // only seems to be one feature bit (breakpoints)
  version_reply->features = 1;
  
  return 1;
}


#define COMMAND_KDP_HOSTINFO 2
struct kdp_packet_hostinfo_request {
  struct kdp_packet_header header;
} __attribute__((packed));

struct kdp_packet_hostinfo_reply {
  struct kdp_packet_header header;
  uint32_t cpus_mask;
  uint32_t cpu_type;
  uint32_t cpu_subtype;
} __attribute__((packed));

int command_kdp_hostinfo(struct kdp_packet_header* request, struct kdp_packet_header* reply) {
  log_request("KDP_HOSTNAME");
  struct kdp_packet_hostinfo_reply* hostinfo_reply = (struct kdp_packet_hostinfo_reply*)reply;
  
  hostinfo_reply->header.total_length = sizeof(struct kdp_packet_hostinfo_reply);
  
  // only expose one cpu:
  hostinfo_reply->cpus_mask = 1;
  
#define CPU_TYPE_ARM (12)
#define CPU_ARCH_ABI64 (0x01000000)
#define CPU_TYPE_ARM64 (CPU_TYPE_ARM | CPU_ARCH_ABI64)
  hostinfo_reply->cpu_type = CPU_TYPE_ARM64;
  
#define CPU_SUBTYPE_ARM64_V8 (1)
  hostinfo_reply->cpu_subtype = CPU_SUBTYPE_ARM64_V8;
  
  return 1;
}

#define COMMAND_KDP_DISCONNECT 1
struct kdp_packet_disconnect_request {
  struct kdp_packet_header header;
} __attribute__((packed));

struct kdp_packet_disconnect_reply {
  struct kdp_packet_header header;
} __attribute__((packed));

int command_kdp_disconnect(struct kdp_packet_header* request, struct kdp_packet_header* reply) {
  log_request("KDP_DISCONNECT");
  struct kdp_packet_disconnect_reply* disconnect_reply = (struct kdp_packet_disconnect_reply*)reply;
  
  // remove all breakpoints:
  disable_all_breakpoints();
  
  disconnect_reply->header.total_length = sizeof(struct kdp_packet_disconnect_reply);
  
  return 1;
}

#define COMMAND_KDP_KERNELVERSION 24
struct kdp_packet_kernelversion_request {
  struct kdp_packet_header header;
} __attribute__((packed));

struct kdp_packet_kernelversion_reply {
  struct kdp_packet_header header;
  char   version[KDP_MAX_INLINE_DATA_SIZE];
} __attribute__((packed));

int command_kdp_kernelversion(struct kdp_packet_header* request, struct kdp_packet_header* reply) {
  log_request("KDP_KERNELVERSION");
  struct kdp_packet_kernelversion_reply* kernelversion_reply = (struct kdp_packet_kernelversion_reply*)reply;
  
  // this is an example kernelversion string on MacOS:
  // "Darwin Kernel Version 17.3.0: Thu Nov  9 18:09:22 PST 2017; root:xnu-4570.31.3~1/RELEASE_X86_64; UUID=87641697-A3DD-30C4-B010-E65ECE57550B; stext=0xffffff802aa00000";
  // there are three parts we need to put together:
  // 1: the global variable version
  // 2: ; UUID=XXX - the XXX are the kernel_uuid_string cstring
  // 3: ; stext=0xXXX - the XXX is the kernel base address (where the 0xfeedfacf is, not the base of the kernelcache)
  
  char kernelversion_string[1024];
  kernelversion_string[0] = 0;
  
  char version[512];
  rkstring(ksym(KSYMBOL_VERSION_STRING), version, sizeof(version));
  
  strcpy(kernelversion_string, version);
  
  char uuid[512];
  rkstring(ksym(KSYMBOL_KERNEL_UUID_STRING), uuid, sizeof(uuid));
  
  strcat(kernelversion_string, "; UUID=");
  strcat(kernelversion_string, uuid);
  
  
  char text_base_string[32];
  sprintf(text_base_string, "%p", (void*)kbase);
  
  strcat(kernelversion_string, "; stext=");
  strcat(kernelversion_string, text_base_string);
  
  printf("kernel version string: %s\n", kernelversion_string);
  
  kernelversion_reply->header.total_length = offsetof(struct kdp_packet_kernelversion_reply, version[0]) + strlen(kernelversion_string) + 1;
  strcpy(kernelversion_reply->version, kernelversion_string);
  
  return 1;
}


#define COMMAND_KDP_READMEM64 20
struct kdp_packet_readmem64_request {
  struct kdp_packet_header header;
  uint64_t address;
  uint32_t count;
} __attribute__((packed));

struct kdp_packet_readmem64_reply {
  struct kdp_packet_header header;
  uint32_t error;
  uint8_t bytes[KDP_MAX_INLINE_DATA_SIZE];
} __attribute__((packed));

int command_kdp_readmem64(struct kdp_packet_header* request, struct kdp_packet_header* reply) {
  log_request("KDP_READMEM64");
  struct kdp_packet_readmem64_request* readmem64_request = (struct kdp_packet_readmem64_request*)request;
  struct kdp_packet_readmem64_reply* readmem64_reply = (struct kdp_packet_readmem64_reply*)reply;
  
  // make sure the request packet is sane:
  if (readmem64_request->header.total_length < sizeof(struct kdp_packet_readmem64_request)) {
    error_fatal("KDP_READMEM64 packet invalid (packet too small)");
  }
  
  // make sure the request is for a sane amount of memory:
  if (readmem64_request->count > sizeof(readmem64_reply->bytes)) {
    error_warn("KDP_READMEM64 packet invalid (requesting too much memory)");
    readmem64_reply->header.total_length = offsetof(struct kdp_packet_readmem64_reply, bytes);
    readmem64_reply->error = KDP_ERROR_BAD_BYTE_COUNT;
    return 1;
  }
  
  uint32_t requested_byte_count = readmem64_request->count;
  uint64_t requested_address = readmem64_request->address;
  
  printf("request to read %d bytes from 0x%llx\n", requested_byte_count, requested_address);
  
  readmem64_reply->header.total_length = offsetof(struct kdp_packet_readmem64_reply, bytes[0]) + requested_byte_count;
  
  rkbuffer(requested_address, readmem64_reply->bytes, requested_byte_count);
  
  readmem64_reply->error = KDP_SUCCESS;
  
  return 1;
}


#define COMMAND_KDP_WRITEMEM64 21
struct kdp_packet_writemem64_request {
  struct kdp_packet_header header;
  uint64_t address;
  uint32_t count;
  uint8_t bytes[KDP_MAX_INLINE_DATA_SIZE];
} __attribute__((packed));

struct kdp_packet_writemem64_reply {
  struct kdp_packet_header header;
  uint32_t error;
} __attribute__((packed));

int command_kdp_writemem64(struct kdp_packet_header* request, struct kdp_packet_header* reply) {
  log_request("KDP_WRITEMEM64");
  struct kdp_packet_writemem64_request* writemem64_request = (struct kdp_packet_writemem64_request*)request;
  struct kdp_packet_writemem64_reply* writemem64_reply = (struct kdp_packet_writemem64_reply*)reply;
  
  // make sure the request packet length is sane:
  if (writemem64_request->header.total_length < offsetof(struct kdp_packet_writemem64_request, bytes)) {
    error_fatal("KDP_WRITEMEM64 packet invalid (packet too small)");
  }
  
  // make sure the count field matches the packet length:
  if (writemem64_request->count != writemem64_request->header.total_length - offsetof(struct kdp_packet_writemem64_request, bytes)) {
    error_fatal("KDP_WRITEMEM64 packet invalid (count field doesn't match size of sent packet");
  }
  
  printf("request to write %d bytes to 0x%llx\n", writemem64_request->count, writemem64_request->address);
  
  writemem64_reply->header.total_length = sizeof(struct kdp_packet_writemem64_reply);
  writemem64_reply->error = KDP_SUCCESS;
  
  // nothing to do for now
  return 1;
}

#define COMMAND_KDP_BREAKPOINT64_SET 22
struct kdp_packet_breakpoint64_set_request {
  struct kdp_packet_header header;
  uint64_t address;
} __attribute__((packed));

struct kdp_packet_breakpoint64_set_reply {
  struct kdp_packet_header header;
  uint32_t error;
} __attribute__((packed));

int command_kdp_breakpoint64_set(struct kdp_packet_header* request, struct kdp_packet_header* reply) {
  log_request("KDP_BREAKPOINT64_SET");
  struct kdp_packet_breakpoint64_set_request* breakpoint64_set_request = (struct kdp_packet_breakpoint64_set_request*)request;
  struct kdp_packet_breakpoint64_set_reply* breakpoint64_set_reply = (struct kdp_packet_breakpoint64_set_reply*)reply;
  
  // make sure the request packet length is sane:
  if (breakpoint64_set_request->header.total_length != sizeof(struct kdp_packet_breakpoint64_set_request)) {
    error_fatal("KDP_BREAKPOINT64_SET packet invalid (packet size incorrect)");
  }
  
  uint64_t breakpoint_address = breakpoint64_set_request->address;
  
  printf("request to set breakpoint at 0x%llx\n", breakpoint_address);
  
  // tell the kdbg code about the breakpoint
  int success = enable_breakpoint(breakpoint_address);
  
  breakpoint64_set_reply->header.total_length = sizeof(struct kdp_packet_breakpoint64_set_reply);
  
  if (success) {
    breakpoint64_set_reply->error = KDP_SUCCESS;
  } else {
    breakpoint64_set_reply->error = KDP_ERROR_TOO_MANY_BREAKPOINTS;
  }
  
  return 1;
}

#define COMMAND_KDP_BREAKPOINT64_REMOVE 23
struct kdp_packet_breakpoint64_remove_request {
  struct kdp_packet_header header;
  uint64_t address;
} __attribute__((packed));

struct kdp_packet_breakpoint64_remove_reply {
  struct kdp_packet_header header;
  uint32_t error;
} __attribute__((packed));

int command_kdp_breakpoint64_remove(struct kdp_packet_header* request, struct kdp_packet_header* reply) {
  log_request("KDP_BREAKPOINT64_REMOVE");
  struct kdp_packet_breakpoint64_remove_request* breakpoint64_remove_request = (struct kdp_packet_breakpoint64_remove_request*)request;
  struct kdp_packet_breakpoint64_remove_reply* breakpoint64_remove_reply = (struct kdp_packet_breakpoint64_remove_reply*)reply;
  
  // make sure the request packet length is sane:
  if (breakpoint64_remove_request->header.total_length != sizeof(struct kdp_packet_breakpoint64_remove_request)) {
    error_fatal("KDP_BREAKPOINT64_REMOVE packet invalid (packet size incorrect)");
  }
  
  uint64_t breakpoint_address = breakpoint64_remove_request->address;
  
  printf("request to remove breakpoint at 0x%llx\n", breakpoint_address);
  int success;
  
  // if we're actually trying to continue or single step, then don't remove the breakpoint
  // we will emulate the instruction, and lldb won't set the bp again because single step doesn't work how it expects
  
  if (do_continue || do_single_step) {
    printf("skipping kdp_breakpoint64_remove inside a do_continue or do_single_step...\n");
    success = 1;
  } else {
    // tell the kdbg code to disable this breakpoint
    success = disable_breakpoint(breakpoint_address);
  }
  breakpoint64_remove_reply->header.total_length = sizeof(struct kdp_packet_breakpoint64_remove_reply);
  
  if (success) {
    breakpoint64_remove_reply->error = KDP_SUCCESS;
  } else {
    breakpoint64_remove_reply->error = KDP_ERROR_TOO_MANY_BREAKPOINTS;
  }
  
  return 1;
}

#define COMMAND_KDP_RESUMECPUS 12
struct kdp_packet_resumecpus_request {
  struct kdp_packet_header header;
  uint32_t cpus_mask;
} __attribute__((packed));

struct kdp_packet_resumecpus_reply {  // no error field
  struct kdp_packet_header header;
} __attribute__((packed));

int command_kdp_resumecpus(struct kdp_packet_header* request, struct kdp_packet_header* reply) {
  log_request("KDP_RESUMECPUS");
  struct kdp_packet_resumecpus_request* resumecpus_request = (struct kdp_packet_resumecpus_request*)request;
  struct kdp_packet_resumecpus_reply* resumecpus_reply = (struct kdp_packet_resumecpus_reply*)reply;
  
  // make sure the request packet length is sane:
  if (resumecpus_request->header.total_length != sizeof(struct kdp_packet_resumecpus_request)) {
    error_fatal("KDP_RESUMECPUS packet invalid (packet size incorrect)");
  }
  
  // we only expose a single cpu, so check that the mask is for that:
  if (resumecpus_request->cpus_mask != 1) {
    error_warn("KDP_RESUMECPUS asked to resume an invalid cpu");
  }
  // nothing to do yet
  
  resumecpus_reply->header.total_length = sizeof(struct kdp_packet_resumecpus_reply);
  
  return 1;
}


#define COMMAND_KDP_READREGS 7
struct kdp_packet_readregs_request {
  struct kdp_packet_header header;
  uint32_t cpu_number;
  uint32_t flavor;
} __attribute__((packed));

#define KDP_ARM_THREAD_STATE64 6

struct kdp_packet_readregs_reply {
  struct kdp_packet_header header;
  uint32_t error;
  struct arm_thread_state64 state;
} __attribute__((packed));

int command_kdp_readregs(struct kdp_packet_header* request, struct kdp_packet_header* reply, struct arm_thread_state64* target_state) {
  log_request("KDP_READREGS");
  struct kdp_packet_readregs_request* readregs_request = (struct kdp_packet_readregs_request*)request;
  struct kdp_packet_readregs_reply* readregs_reply = (struct kdp_packet_readregs_reply*)reply;
  
  // make sure the request packet is sane:
  if (readregs_request->header.total_length < sizeof(struct kdp_packet_readregs_request)) {
    error_fatal("KDP_READREGS packet invalid (packet too small)");
  }
  
  // make sure the flavour is correct
  if (readregs_request->flavor != KDP_ARM_THREAD_STATE64) {
    error_warn("KDP_READREGS packet invalid (unknown state flavour)");
    readregs_reply->header.total_length = offsetof(struct kdp_packet_readregs_reply, state);
    readregs_reply->error = KDP_ERROR_UNKNOWN_STATE_FLAVOR;
    return 1;
  }
  
  readregs_reply->header.total_length = sizeof(struct kdp_packet_readregs_reply);
  
  if (target_state) {
    memcpy(&readregs_reply->state, target_state, sizeof(struct arm_thread_state64));
  } else {
    // this is actually an attach and we haven't really stopped anything
    // fake something which looks reasonable.
    memset(&readregs_reply->state, 'A', sizeof(readregs_reply->state));
    readregs_reply->state.pc = ksym(KSYMBOL_EXCEPTION_RETURN);
  }
  readregs_reply->error = KDP_SUCCESS;
  
  return 1;
}


#define COMMAND_KDP_WRITEREGS 8
struct kdp_packet_writeregs_request {
  struct kdp_packet_header header;
  uint32_t cpu_number;
  uint32_t flavor;
  struct arm_thread_state64 state;
} __attribute__((packed));

struct kdp_packet_writeregs_reply {
  struct kdp_packet_header header;
  uint32_t error;
} __attribute__((packed));

int command_kdp_writeregs(struct kdp_packet_header* request, struct kdp_packet_header* reply, struct arm_thread_state64* target_state) {
  log_request("KDP_WRITEREGS");
  struct kdp_packet_writeregs_request* writeregs_request = (struct kdp_packet_writeregs_request*)request;
  struct kdp_packet_writeregs_reply* writeregs_reply = (struct kdp_packet_writeregs_reply*)reply;
  
  // make sure the request packet is sane:
  if (writeregs_request->header.total_length < sizeof(struct kdp_packet_writeregs_request)) {
    error_fatal("KDP_WRITEREGS packet invalid (packet too small)");
  }
  
  // make sure the flavour is correct
  if (writeregs_request->flavor != KDP_ARM_THREAD_STATE64) {
    error_warn("KDP_WRITEREGS packet invalid (unknown state flavour)");
    writeregs_reply->header.total_length = sizeof(struct kdp_packet_writeregs_reply);
    writeregs_reply->error = KDP_ERROR_UNKNOWN_STATE_FLAVOR;
    return 1;
  }
  
  memcpy(target_state, &writeregs_request->state, sizeof(struct arm_thread_state64));
  
  writeregs_reply->header.total_length = sizeof(struct kdp_packet_writeregs_reply);
  writeregs_reply->error = KDP_SUCCESS;
  
  return 1;
}

// custom command - we're overriding the command number of KDP_READIOPORT
#define COMMAND_KDP_KERNEL_CONTINUE 27
struct kdp_packet_kernel_continue_request {
  struct kdp_packet_header header;
} __attribute__((packed));

struct kdp_packet_kernel_continue_reply {
  struct kdp_packet_header header;
} __attribute__((packed));

int command_kdp_kernel_continue(struct kdp_packet_header* request, struct kdp_packet_header* reply) {
  log_request("KDP_KERNEL_CONTINUE");
  struct kdp_packet_kernel_continue_request* kernel_continue_request = (struct kdp_packet_kernel_continue_request*)request;
  struct kdp_packet_kernel_continue_reply* kernel_continue_reply = (struct kdp_packet_kernel_continue_reply*)reply;
  
  // make sure the request packet length is sane:
  if (kernel_continue_request->header.total_length != sizeof(struct kdp_packet_kernel_continue_request)) {
    error_fatal("KDP_KERNEL_CONTINUE packet invalid (packet size incorrect)");
  }
  
  do_continue = 1;
  
  kernel_continue_reply->header.total_length = sizeof(struct kdp_packet_kernel_continue_reply);
  
  return 1;
}

// this is overriding the command numer of KDP_WRITEIOPORT
#define COMMAND_KDP_KERNEL_SINGLE_STEP 28
struct kdp_packet_kernel_single_step_request {
  struct kdp_packet_header header;
} __attribute__((packed));

struct kdp_packet_kernel_single_step_reply {
  struct kdp_packet_header header;
} __attribute__((packed));

int command_kdp_kernel_single_step(struct kdp_packet_header* request, struct kdp_packet_header* reply) {
  log_request("KDP_KERNEL_SINGLE_STEP");
  struct kdp_packet_kernel_single_step_request* kernel_single_step_request = (struct kdp_packet_kernel_single_step_request*)request;
  struct kdp_packet_kernel_single_step_reply* kernel_single_step_reply = (struct kdp_packet_kernel_single_step_reply*)reply;
  
  // make sure the request packet length is sane:
  if (kernel_single_step_request->header.total_length != sizeof(struct kdp_packet_kernel_single_step_request)) {
    error_fatal("KDP_KERNEL_SINGLE_STEP packet invalid (packet size incorrect)");
  }
  
  do_single_step = 1;
  
  kernel_single_step_reply->header.total_length = sizeof(struct kdp_packet_kernel_single_step_reply);
  
  return 1;
}

#define SERVER_LOOP_EXIT_REASON_NONE 0
#define SERVER_LOOP_EXIT_REASON_RESUME 1
#define SERVER_LOOP_EXIT_REASON_DISCONNECT 2
#define SERVER_LOOP_EXIT_REASON_SINGLE_STEP 3

/*
 * this loops parses the commands involved in an interactive session
 *
 * exiting this loop implies that the debugee should continue, the return
 * value indicates the reason for exiting the loop
 */
int server_fd = -1;
uint16_t connection_reply_port = 0;
uint16_t connection_exception_port = 0;
struct in_addr connection_remote_address = {0};

int kdp_server_loop(struct arm_thread_state64* target_state) {
  for (;;) {
    struct in_addr remote_address = {0};
    
    uint8_t request_buffer[KDP_MAX_REQUEST_SIZE] = {0};
    uint8_t reply_buffer[KDP_MAX_REQUEST_SIZE] = {0};
    
    ssize_t received_count = receive_packet(server_fd, request_buffer, KDP_MAX_REQUEST_SIZE, &remote_address);
    if (received_count == -1) {
      error_warn("server loop got invalid packet");
      continue;
    }
    
    struct kdp_packet_header* request_header = (struct kdp_packet_header*)request_buffer;
    struct kdp_packet_header* reply_header = (struct kdp_packet_header*)reply_buffer;
    
    int has_reply = 0;
    int exit_server_loop = 0;
    switch(request_header->command) {
      case COMMAND_KDP_VERSION:
      {
        has_reply = command_kdp_version(request_header, reply_header);
        break;
      }
      case COMMAND_KDP_HOSTINFO:
      {
        has_reply = command_kdp_hostinfo(request_header, reply_header);
        break;
      }
      case COMMAND_KDP_KERNELVERSION:
      {
        has_reply = command_kdp_kernelversion(request_header, reply_header);
        break;
      }
      case COMMAND_KDP_READMEM64:
      {
        has_reply = command_kdp_readmem64(request_header, reply_header);
        break;
      }
      case COMMAND_KDP_WRITEMEM64:
      {
        has_reply = command_kdp_writemem64(request_header, reply_header);
        break;
      }
      case COMMAND_KDP_READREGS:
      {
        has_reply = command_kdp_readregs(request_header, reply_header, target_state);
        break;
      }
      case COMMAND_KDP_WRITEREGS:
      {
        has_reply = command_kdp_writeregs(request_header, reply_header, target_state);
        break;
      }
      case COMMAND_KDP_BREAKPOINT64_SET:
      {
        has_reply = command_kdp_breakpoint64_set(request_header, reply_header);
        break;
      }
      case COMMAND_KDP_BREAKPOINT64_REMOVE:
      {
        has_reply = command_kdp_breakpoint64_remove(request_header, reply_header);
        break;
      }
      case COMMAND_KDP_RESUMECPUS:
      {
        has_reply = command_kdp_resumecpus(request_header, reply_header);
        if (do_continue) {
          exit_server_loop = SERVER_LOOP_EXIT_REASON_RESUME;
          do_continue = 0;
        } else if (do_single_step) {
          exit_server_loop = SERVER_LOOP_EXIT_REASON_SINGLE_STEP;
          do_single_step = 0;
        } else {
          exit_server_loop = SERVER_LOOP_EXIT_REASON_RESUME;
        }
        break;
      }
      case COMMAND_KDP_DISCONNECT:
      {
        has_reply = command_kdp_disconnect(request_header, reply_header);
        exit_server_loop = SERVER_LOOP_EXIT_REASON_DISCONNECT;
        break;
      }
      case COMMAND_KDP_KERNEL_CONTINUE:
      {
        has_reply = command_kdp_kernel_continue(request_header, reply_header);
        break;
      }
      case COMMAND_KDP_KERNEL_SINGLE_STEP:
      {
        has_reply = command_kdp_kernel_single_step(request_header, reply_header);
        break;
      }
      default:
      {
        error_warn("unrecognized KDP command");
        printf("%d\n", request_header->command);
        break;
      }
    }
    
    if (has_reply) {
      // fill the header:
      // the message handler has to fill the total_length field
      
      if (reply_header->total_length == 0) {
        error_fatal("message handler didn't fill in the total_length field");
      }
      
      reply_header->command = request_header->command;
      reply_header->is_reply = 1;
      reply_header->sequence_number = request_header->sequence_number;
      reply_header->session_key = request_header->session_key;
      
      printf("about to send reply to: %s : %d\n", inet_ntoa(remote_address), connection_reply_port);
      
      // send the reply
      send_packet(server_fd, reply_header, reply_header->total_length, remote_address, connection_reply_port);
    }
    
    // should we exit the server loop?
    if (exit_server_loop) {
      return exit_server_loop;
    }
  }
}

// this starts up the kdp listener, waits for a connection then
// calls the server_loop to handle the initial interactive commands until the client
// either disconnects or resumes execution
// because this happens before the target has actually started running instrumented code
// you can't modify the initial register state.
// You can however set breakpoints and modify memory.

// if this returns EXIT_SERVER_LOOP_REASON_DISCONNECT then the debugger should be disabled
// otherwise debugger-enabled execution should start

// the interactive loop will only be entered again once a breakpoint is hit

// this should run on the thread which will be debugged

int start_kdp_server() {
  server_fd = create_server_socket(kdp_well_known_port);
  if (server_fd == 0) {
    error_fatal("unable to start server");
  }
  
  // do the kdp handshake to establish a connection:
  
  // wait for a KDP_REATTACH message
  printf("waiting for remote connection\n");
  
  struct in_addr remote_address = {0};
  
  struct kdp_packet_reattach_request reattach_request;
  
  ssize_t received_size = expect_packet(COMMAND_KDP_REATTACH,
                                        server_fd,
                                        &reattach_request,
                                        sizeof(reattach_request),
                                        sizeof(reattach_request),
                                        &remote_address);
  if (received_size == -1) {
    error_fatal("protocol error: first packet wasn't a valid KDP_REATTACH");
  }
  
  uint16_t reattach_reply_port = ntohs(reattach_request.reply_port);
  printf("reattach_reply_port: %d\n", reattach_reply_port);
  
  // send a reply:
  struct kdp_packet_reattach_reply reply;
  
  reply.header.command = COMMAND_KDP_REATTACH;
  reply.header.is_reply = 1;
  reply.header.sequence_number = reattach_request.header.sequence_number;
  reply.header.total_length = sizeof(reply);
  reply.header.session_key = reattach_request.header.session_key; //new_session_key();
  
  send_packet(server_fd, (uint8_t*)&reply, sizeof(reply), remote_address, reattach_reply_port);
  
  
  // wait for a KDP_CONNECT packet
  struct kdp_packet_connect_request connect_request;
  memset(&connect_request, 0, sizeof(connect_request));
  
  received_size = expect_packet(COMMAND_KDP_CONNECT,
                                server_fd,
                                &connect_request,
                                offsetof(struct kdp_packet_connect_request, greeting[1]),
                                sizeof(connect_request),
                                &remote_address);
  
  if (received_size == -1) {
    error_fatal("protocol error: second packet wasn't a valid KDP_CONNECT");
  }
  
  // grab the reply and exception ports
  connection_reply_port = ntohs(connect_request.reply_port);
  connection_exception_port = ntohs(connect_request.exception_port);
  connection_remote_address = remote_address;
  
  printf("connection_reply_port: %d\n", connection_reply_port);
  
  // ensure the greeting is NULL terminated:
  connect_request.greeting[sizeof(connect_request.greeting)-1] = 0;
  
  printf("got KDP_CONNECT packet with following greeting: %s\n", connect_request.greeting);
  
  // send a reply:
  struct kdp_packet_connect_reply connect_reply;
  
  connect_reply.header.command = COMMAND_KDP_CONNECT;
  connect_reply.header.is_reply = 1;
  connect_reply.header.sequence_number = connect_request.header.sequence_number;
  connect_reply.header.total_length = sizeof(connect_reply);
  connect_reply.header.session_key = connect_request.header.session_key; // current_session_key();
  
  connect_reply.error = 0;
  send_packet(server_fd, (uint8_t*)&connect_reply, sizeof(connect_reply), remote_address, connection_reply_port);
  
  // enter the server loop:
  int server_loop_exit_reason = kdp_server_loop(NULL);
  
  return server_loop_exit_reason;
}

#define COMMAND_KDP_EXCEPTION 13
struct kdp_packet_exception_request {
  struct kdp_packet_header header;
  uint32_t count_exceptions;
  uint32_t cpu_number;
  uint32_t exception_type;
  uint32_t code;
  uint32_t subcode;
} __attribute__((packed));

struct kdp_packet_exception_reply {
  struct kdp_packet_header header;
} __attribute__((packed));

typedef uint8_t (*read_byte_handler)(uint64_t address);
typedef void(*write_byte_handler)(uint64_t address, uint8_t value);

int arm64_emulate(struct arm_thread_state64* native_state, read_byte_handler read_func, write_byte_handler write_func);

uint8_t emulation_read_byte(uint64_t address) {
  printf("emulation request to read byte at 0x%llx\n", address);
  //return kread32(address) & 0xff;
  uint8_t byte = 0;
  kmemcpy((uint64_t)&byte, address, 1);
  return byte;
}

void emulation_write_byte(uint64_t address, uint8_t value) {
  printf("skipping write\n");
  return;
  printf("emulation request to write byte at 0x%llx : 0x%02x\n", address, value);
  
  uint32_t offset = address & 7;
  printf("offset: %d\n", offset);
  
  uint64_t qword_address = address & ~(7ULL);
  printf("qword address: 0x%016llx\n", qword_address);
  
  uint64_t qword_value = kread64(qword_address);
  printf("qword value: 0x%016llx\n", qword_value);
  
  uint64_t mask = 0xffULL << (offset*8);
  printf("mask: 0x%016llx\n", mask);
  
  qword_value &= ~mask;
  printf("cleared mask: %016llx\n", qword_value);
  
  uint64_t value_to_or = (((uint64_t)value) << (offset*8));
  printf("value to or: %016llx\n", value_to_or);
  
  qword_value |= value_to_or;
  printf("or'ed in bytes: %016llx\n", qword_value);
  
  kwrite64(qword_address, qword_value);
  printf("wrote back\n");
#if 0
  //uint32_t orig = kread32(address);
  //orig &= 0xffffff00;
  //orig |= value;
  //kwrite32(address, orig);
  kmemcpy(address, (uint64_t)&value, 1);
  
  // try to read it back:
  uint8_t read_back = 0;
  kmemcpy((uint64_t)&read_back, address, 1);
  
  if (read_back != value) {
    printf(" ******* read back value 0x%02x differs from that which we tried to write: 0x%02x\n", read_back, value);
  } else {
    printf("read back value matches\n");
  }
#endif
}

int expecting_step_stop = 0;
int step_stop_is_also_regular_bp = 0;
uint64_t step_stop_addr = 0;
uint64_t prev_bp = 0;
int force_manual_single_step = 0;

// this is called by the kdbg when a breakpoint is hit
void kdp_handle_stop(arm_context_t* state) {
  printf("kdp_handle_stop\n");
  struct arm_thread_state64* thread_state = (struct arm_thread_state64*)(&(state->ss.ss_64));
  
  uint64_t stop_pc = thread_state->pc;
  printf("stop_pc: 0x%016llx\n", stop_pc);
  
  int server_loop_exit_reason = SERVER_LOOP_EXIT_REASON_NONE;
  
  int send_exception_message = 0;
  
  // expecting_step_stop will be true if we requested a hardware single-step before the last resume
  if (expecting_step_stop) {
    printf("hit single-step stop\n");
    
    // re-enable the previous bp, if there was one
    // this is for the case when we're resuming execution after hitting a breakpoint
    // we need to temporarily remove that breakpoint, do a single-step and then set that bp again
    if (prev_bp) {
      printf("re-enabling previous breakpoint: 0x%016llx\n", prev_bp);
      enable_breakpoint(prev_bp);
      prev_bp = 0;
    }
    
    expecting_step_stop = 0;
    
    // this single-step stop is also a breakpoint
    // we single-stepped on to a normal bp, so send an exception message
    if (breakpoint_is_enabled(stop_pc)) {
      send_exception_message = 1;
    }
    
    // we marked this single-step as being user-initiated so send an exception message
    if (force_manual_single_step) {
      send_exception_message = 1;
      force_manual_single_step = 0;
    }
  } else {
    // we were not expecting a single-setp stop so this is a regular breakpoint event
    send_exception_message = 1;
  }
  
  if (send_exception_message) {
    // send an exception message to the client
    struct kdp_packet_exception_request request;
    request.header.command = COMMAND_KDP_EXCEPTION;
    request.header.is_reply = 0;
    request.header.sequence_number = 0;
    request.header.total_length = sizeof(struct kdp_packet_exception_request);
    request.header.session_key = 0;
    
    request.count_exceptions = 1;
    request.cpu_number = 1;
#define EXCEPTION_TYPE_BREAKPOINT 6
#define EXCEPTION_CODE_ARM_BREAKPOINT 1
    request.exception_type = EXCEPTION_TYPE_BREAKPOINT;
    request.code = EXCEPTION_CODE_ARM_BREAKPOINT;
    request.subcode = EXCEPTION_CODE_ARM_BREAKPOINT;
    
    printf("sending breakpoint hit notification message\n");
    
    send_packet(server_fd, &request, sizeof(request), connection_remote_address, connection_exception_port);
    
    // receive the reply
    struct in_addr reply_remote_address;
    struct kdp_packet_exception_reply reply;
    ssize_t received_size = expect_packet(COMMAND_KDP_EXCEPTION,
                                          server_fd,
                                          &reply,
                                          sizeof(reply),
                                          sizeof(reply),
                                          &reply_remote_address);
    
    if (received_size != sizeof(reply)) {
      error_fatal("protocol error: invalid reply to exception notification message");
    }
    
    // enter the server loop with that state
    // this starts an interactive session with the client where thread_state might be modified
    server_loop_exit_reason = kdp_server_loop(thread_state);
    
    // check the server loop exit reason:
    if (server_loop_exit_reason == SERVER_LOOP_EXIT_REASON_DISCONNECT) {
      printf("client disconnected, need to disable all breakpoints\n");
      // for a disconnect we need to disable all the breakpoints and try to clean up the mess we made
      // not supported yet ;)
    }
  }
  
  // prepare to resume execution
  // if there is a breakpoint enabled on the current pc then we'll need to remove it, single step then
  // add it back
  // we will also need to single-step if the user explicitly requested a single-step
  if (breakpoint_is_enabled(stop_pc) || server_loop_exit_reason == SERVER_LOOP_EXIT_REASON_SINGLE_STEP) {
    // record that the next stop will be a single-step
    expecting_step_stop = 1;
    
    // if a breakpoint is set at the current pc, record its address then disable it
    if (breakpoint_is_enabled(stop_pc)) {
      prev_bp = stop_pc;
      printf("temporarily disabling breakpoint at stop_pc 0x%016llx for single-step\n", stop_pc);
      disable_breakpoint(stop_pc);
    } else {
      prev_bp = 0;
    }
    
    if (server_loop_exit_reason == SERVER_LOOP_EXIT_REASON_SINGLE_STEP) {
      force_manual_single_step = 1;
    }
    
    // force a single-step:
    thread_state->cpsr |= (1<<21); // SPSR_SS
    
    // the caller of this function will look to see whether that bit is set, and if so will also ensure that MDSCR_EL1.ss_enable is set
  }
}
