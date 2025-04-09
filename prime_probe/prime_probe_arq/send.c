#include "send.h"
#include "phy.h"             
#include "covert_utils.h"    // for frame_t, construct_frame
#include <stdio.h>

void send_bit(bool bit) {
    phy_send_bit(bit);
}

void send_bits(bool *bits, size_t num_bits) {
    for (size_t i = 0; i < num_bits; i++) {
        phy_send_bit(bits[i]);
    }
}

void send_byte_frame(uint8_t byte)
{
    frame_t frame = construct_frame(byte);

    send_bits(frame.start_delimiter, 8);
    send_bits(frame.data, 8);
    send_bit(frame.parity);

    printf("[send_byte_frame] Sent byte: 0x%x ('%c')\n", byte, byte);
}
