#ifndef _ARM_THREAD_STATE64_H
#define _ARM_THREAD_STATE64_H

struct arm_thread_state64 {
  uint64_t x[29];
  uint64_t fp;
  uint64_t lr;
  uint64_t sp;
  uint64_t pc;
  uint32_t cpsr;
  uint32_t padding;
} __attribute__((packed));

#endif
