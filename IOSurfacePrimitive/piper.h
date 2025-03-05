#include <stdio.h>
#include <stdlib.h>

int* create_pipes(void);

size_t
pipe_spray(const int *pipefds, size_t pipe_count,
        void *pipe_buffer, size_t pipe_buffer_size);

void
pipe_close(int pipefds[2]);