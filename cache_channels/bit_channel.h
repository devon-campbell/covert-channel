#ifndef BIT_CHANNEL_H
#define BIT_CHANNEL_H

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>


void calibrate_bit_channel(void);

bool receive_bit(void *addr);
void send_bit(void *addr, bool bit);

static inline void send_bits(bool *bits, size_t num_bits, void *addr)
{
    for (size_t i = 0; i < num_bits; i++)
    {
        send_bit(addr, bits[i]);
    }
}

#endif // BIT_CHANNEL_H