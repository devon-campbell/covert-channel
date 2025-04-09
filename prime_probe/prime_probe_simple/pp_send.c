/***************************  pp_send.c  ******************************/
#include "pp_common.h"
#include <stdio.h>
#include <unistd.h>

static inline void wait_slot_start(void)
{
    uint32_t t;
    do {
        t = get_cycles32() & SLOT_MASK;
    } while (t > 500);  // tight sync on slot boundary
}

void send_bit(int bit)
{
    wait_slot_start();
    prime_cache_set();

    if (bit) {
        /* keep evicting during first half‑window */
        uint32_t half = SLOT_MASK >> 1;
        while ((get_cycles32() & SLOT_MASK) < half)
            evict_cache_set();
    } else {
        while ((get_cycles32() & SLOT_MASK) < (SLOT_MASK >> 1));
    }
    /* second half reserved for receiver probe */
}

int main(void)
{
    pp_init();
    // prime_probe_lines[0] = (volatile char*) 0x7b8ad60000c0;  // hardcode same address in both
    printf("[pp] manually set line 0 to virt %p\n", prime_probe_lines[0]);


    puts("[sender] streaming 1010… (CTRL‑C to stop)");
    int bit = 1;
    while (1) {
        send_bit(bit);
        // bit ^= 1;
    }
}