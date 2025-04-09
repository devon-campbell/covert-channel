#ifndef SEND_H
#define SEND_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Prime+Probe send-side logic
void send_bit(bool bit);
void send_bits(bool *bits, size_t num_bits);
void send_byte_frame(uint8_t byte);

#endif // SEND_H
