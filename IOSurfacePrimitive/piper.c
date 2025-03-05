#include "piper.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>

void
pipe_close(int pipefds[2]) {
    close(pipefds[0]);
    close(pipefds[1]);
}

int *
create_pipes(void) {
    // Allocate our initial array.
    size_t capacity = 1;
    int *pipefds = calloc(2 * capacity, sizeof(int));
    // Create as many pipes as we can.
    size_t count = 0;
    for (; count < capacity; count++) {
        // First create our pipe fds.
        int fds[2] = { -1, -1 };
        int error = pipe(fds);
        // Unfortunately pipe() seems to return success with invalid fds once we've
        // exhausted the file limit. Check for this.
        if (error != 0 || fds[0] < 0 || fds[1] < 0) {
            pipe_close(fds);
            break;
        }
        // Mark the write-end as nonblocking.
        //set_nonblock(fds[1]);
        // Store the fds.
        pipefds[2 * count + 0] = fds[0];
        pipefds[2 * count + 1] = fds[1];
    }
    assert(count == capacity && "can't alloc enough pipe fds");
    // Truncate the array to the smaller size.
    int *new_pipefds = realloc(pipefds, 2 * count * sizeof(int));
    assert(new_pipefds != NULL);
    // Return the count and the array.
    // *pipe_count = count;
    return new_pipefds;
}

size_t
pipe_spray(const int *pipefds, size_t pipe_count,
        void *pipe_buffer, size_t pipe_buffer_size) {
    assert(pipe_count <= 0xffffff);
    assert(pipe_buffer_size > 512);
    size_t write_size = pipe_buffer_size - 1;
    size_t pipes_filled = 0;
    for (size_t i = 0; i < pipe_count; i++) {
        // Update the buffer.
        // if (update != NULL) {
        //     update((uint32_t)i, pipe_buffer, pipe_buffer_size);
        // }
        // Fill the write-end of the pipe with the buffer. Leave off the last byte.
        int wfd = pipefds[2 * i + 1];
        ssize_t written = write(wfd, pipe_buffer, write_size);
        if (written != write_size) {
            // This is most likely because we've run out of pipe buffer memory. None of
            // the subsequent writes will work either.
            break;
        }
        pipes_filled++;
    }
    return pipes_filled;
}