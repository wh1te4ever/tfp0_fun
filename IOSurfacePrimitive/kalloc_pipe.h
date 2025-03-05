#include <stdint.h>

typedef struct {
    int fd[2];
    void *user_buffer;
    uint64_t kern_buffer;
    size_t size;
} pipe_info_t;

pipe_info_t *kalloc_via_pipe(size_t size);

void kfree_via_pipe(pipe_info_t *info);