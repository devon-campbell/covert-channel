#ifndef SEND_H
#define SEND_H

#include "covert_utils.h"

void send_bit(void *target_address, bool bit);
void send_bits(bool *bits, size_t num_bits, void *target_address);
void send_byte_frame(void *target_address, uint8_t byte);

#endif