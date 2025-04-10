#ifndef RECEIVE_H
#define RECEIVE_H
#include "covert_utils.h"


static inline uint32_t measure_one_block_access_time(uint64_t addr)
{
    uint32_t cycles;

    asm volatile("mov %1, %%r8\n\t"
                 "lfence\n\t"
                 "rdtsc\n\t"
                 "mov %%eax, %%edi\n\t"
                 "mov (%%r8), %%r8\n\t"
                 "lfence\n\t"
                 "rdtsc\n\t"
                 "sub %%edi, %%eax\n\t"
                 : "=a"(cycles) /*output*/
                 : "r"(addr)
                 : "r8", "edi");

    return cycles;
}

bool receive_bit(void *target_address);

frame_t *receive_byte_frame(void *target_address, frame_t *frame_buf, uint64_t timeout);

uint64_t tune_threshold();

static uint64_t det_threshold = 0;
static inline uint64_t get_threshold(){
    if (det_threshold == 0) {
        printf("Initial threshold detection...\n");
        det_threshold = tune_threshold();
        printf("Detection threshold (cycles): %lu\n", det_threshold);
    }
    return det_threshold;
}

#endif