#include "send.h"
#include "print_utils.h"
#include <sched.h>

#define RAS_SIZE 16

static inline void flush_ras(int count, int threshold){
    if (count == threshold) {
        sched_yield();
        return;
    }
    else flush_ras(++count, threshold);
}

// Send either high or low for a given number of cycles
static inline void send_bit(bool bit){
    // Wait until time step A
    uint32_t initial = start_sync();

    if (bit){
        // Send until time step B
        while (!is_half_point()){
            flush_ras(0, RAS_SIZE);
        }
    }else{
        while (!is_half_point())
                ;
    }
}

// Send a stream of bits
inline void send_bits(bool *bits, size_t num_bits){
    for (size_t i = 0; i < num_bits; i++)    {
        send_bit(bits[i]);
    }
}

// Send a byte frame with the following format:
// |4 bit start delimiter | 8 bit data | 1 bit parity |
// Total length: 13 bits
inline void send_byte_frame(void *target_address, uint8_t byte)
{
    struct frame frame = construct_frame(byte);

    // Send start delimiter
    send_bits(frame.start_delimiter, sizeof(frame.start_delimiter));
    // Send data
    send_bits(frame.data, sizeof(frame.data));
    // Send parity
    send_bit(frame.parity);

    // // Print the frame for debugging
    // print_frame(frame);
    printf("[send_byte_frame] Sent byte: %x\n", byte);

}