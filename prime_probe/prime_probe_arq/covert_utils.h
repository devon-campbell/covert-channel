#ifndef COVERT_UTILS_H
#define COVERT_UTILS_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <x86intrin.h> 

#define START_DELIMITER (0b10011010)

typedef struct frame {
    bool start_delimiter[8];
    bool data[8];
    bool parity;
} frame_t;

#define FRAME_BITLEN (17)

static inline uint64_t rdtscp64(void)
{
    unsigned aux;
    return __rdtscp(&aux);
}


static inline uint8_t byte_from_bools(bool *bits) {
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        byte |= (bits[i] << (7 - i));
    }
    return byte;
}

static inline void byte_to_bools(uint8_t byte, bool *bits) {
    for (int i = 0; i < 8; i++) {
        bits[i] = (byte >> (7 - i)) & 1;
    }
}

static inline bool calculate_parity(bool *data, int size) {
    bool parity = 0;
    for (int i = 0; i < size; i++) {
        parity ^= data[i];
    }
    return parity;
}

static inline bool valid_frame(frame_t f) {
    return byte_from_bools(f.start_delimiter) == START_DELIMITER &&
           calculate_parity(f.data, 8) == f.parity;
}

static inline frame_t construct_frame(uint8_t data) {
    frame_t f = {0};
    byte_to_bools(START_DELIMITER, f.start_delimiter);
    byte_to_bools(data, f.data);
    f.parity = calculate_parity(f.data, 8);
    return f;
}

static inline void print_bools(bool *bits, int size) {
    for (int i = 0; i < size; i++) {
        printf("%d", bits[i]);
        if (i == 3) printf(" ");
    }
}

static inline void print_frame(frame_t f) {
    printf("Frame: | Start Delim: 0x%02X | Data: 0x%02X (%c) | Parity: %d (true=%d)\n",
        byte_from_bools(f.start_delimiter),
        byte_from_bools(f.data),
        byte_from_bools(f.data),
        f.parity,
        calculate_parity(f.data, 8));

    printf("Bits: ");
    print_bools(f.start_delimiter, 8);
    print_bools(f.data, 8);
    printf("%d\n\n", f.parity);
    fflush(stdout);
}

#endif // COVERT_UTILS_H
