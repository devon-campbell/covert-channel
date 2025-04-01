#ifndef PACKET_CHANNEL_H
#define PACKET_CHANNEL_H

#include <stdint.h>

#define CHECKSUM_LENGTH 2

// Simple data packets (up to 256 bytes) with heavy forward error correction, integrity checks, and retransmission
typedef struct packet_header {
    uint8_t flags;
    uint8_t length;
    uint8_t sequence_number; // 0-255, wraps around
    uint8_t checksum[CHECKSUM_LENGTH];
    uint8_t data[16]; // 16 bytes of data
} packet_t;



#endif // PACKET_CHANNEL_H