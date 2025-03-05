#include <mach/mach.h>
#include <stdio.h>
#include <mach-o/loader.h>
#include <unistd.h>
#include <inttypes.h>
#include <errno.h>
#include <stdlib.h> 
#include <unistd.h>

task_t tfp0;
void *libkernrw;

int *pipefds;
uint8_t *pipe_buffer;
size_t pipe_buffer_size;

uint64_t IOSurfaceRootUserClient_addr;

void read_pipe();
void write_pipe();