#include <unistd.h>

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

int physreadbuf(uint64_t pa, void* output, size_t size);
uint64_t physread64(uint64_t pa);
uint32_t physread32(uint64_t pa);

int physwritebuf(uint64_t pa, const void* input, size_t size);
int physwrite8(uint64_t pa, uint8_t v);

int physrw_handoff(pid_t pid);