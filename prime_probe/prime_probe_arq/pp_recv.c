/***************************  pp_recv.c  ******************************/
#define _POSIX_C_SOURCE 200809L
#include "pp_common.h"
#include <stdio.h>
#include <unistd.h>

static uint64_t calibrate_threshold(void)
{
    const int rounds = 100;
    uint64_t sum_p = 0, sum_e = 0;

    for (int r = 0; r < rounds; r++) {
        prime_cache_set();
        sum_p += probe_cache_set();

        evict_cache_set();
        sum_e += probe_cache_set();
    }
    uint64_t t0 = sum_p / rounds;
    uint64_t t1 = sum_e / rounds;
    uint64_t thr = t0 + (t1 - t0) / 2;

    printf("[recv] primed=%lu  evicted=%lu  thr=%lu\n", t0, t1, thr);
    return thr;
}

static int receive_bit(uint64_t thr)
{
    while ((get_cycles32() & SLOT_MASK) > 1000);
    prime_cache_set();
    while ((get_cycles32() & SLOT_MASK) < (SLOT_MASK >> 1));
    return probe_cache_set() > thr;
}

int main(void)
{
    pp_init();
    // prime_probe_lines[0] = (volatile char*) 0x7b8ad60000c0;  // hardcode same address in both
    printf("[pp] manually set line 0 to virt %p\n", prime_probe_lines[0]);


    uint64_t thr = calibrate_threshold();

    puts("[recv] receiving… (CTRL‑C to quit)");
    while (1) {
        int b = receive_bit(thr);
        putchar(b ? '1' : '0');
        fflush(stdout);
    }
}
