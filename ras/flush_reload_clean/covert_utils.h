#ifndef COVERT_UTILS_H
#define COVERT_UTILS_H

#include <stdint.h>
#include <x86intrin.h> // for _mm_clflush, __rdtscp
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include <ctype.h>
#include <assert.h>
#include <fcntl.h>     // for open
#include <sys/mman.h>  // for mmap
#include <sys/types.h> // for size_t
#include <sys/stat.h>  // for open
#include <errno.h>     // for errno
#include <dlfcn.h>



/* TIMING UTILS */

// For coordinating sender and receiver
#define CHANNEL_SYNC_MAX 0x000FFFFF
#define CHANNEL_HALF_MAX (CHANNEL_SYNC_MAX >> 1)
#define CHANNEL_SYNC_JITTER 0x1000

// #define CHANNEL_SYNC_MAX 0xFFFFFF
// #define CHANNEL_HALF_MAX (CHANNEL_SYNC_MAX >> 1)
// #define CHANNEL_SYNC_JITTER 0x100

static inline uint64_t rdtscp64()
{
    unsigned int max;
    return __rdtscp(&max);
}


// Get the current time in cycles
static inline __attribute__((always_inline)) uint32_t get_cycles(void)
{
    uint32_t cycles;
    asm volatile("rdtscp" : /* outputs */ "=a"(cycles));

    return cycles;
}

static inline __attribute__((always_inline)) uint32_t start_sync()
{
    // Spin until time stamp counter overflows
    while ((get_cycles() & CHANNEL_SYNC_MAX) > CHANNEL_SYNC_JITTER)
    {
    }
    return get_cycles();
}

static inline bool __attribute__((always_inline)) is_half_point()
{
    // If counter greater than half max, we are in the second half of the cycle
    return (get_cycles() & CHANNEL_SYNC_MAX) > CHANNEL_HALF_MAX;
}

/* EVENT ADDRESS UTILS */

#define DEFAULT_FILE_NAME "/bin/ls"
#define DEFAULT_FILE_SIZE 4096

static void *event_address = NULL;
static inline void *get_event_address(int offset, const char *shared_filename)
{
    if (event_address != NULL)
    {
        return event_address; // Return cached address if already set
    }

    char *filename;
    if (shared_filename == NULL)
    {
        filename = DEFAULT_FILE_NAME;
    }

    int inFile = open(filename, O_RDONLY);
    if (inFile == -1)
    {
        printf("Failed to Open File\n");
        exit(1);
    }

    void *mapaddr = mmap(NULL, DEFAULT_FILE_SIZE, PROT_READ, MAP_SHARED, inFile, 0);

    if (mapaddr == (void *)-1)
    {
        printf("Failed to Map Address\n");
        exit(1);
    }

    event_address = (void *)((uint64_t)mapaddr + offset);

    return event_address;
}

inline __attribute__((always_inline)) void flush_event(uint64_t addr)
{
    asm volatile("clflush (%0)" ::"r"(addr));
}


/* FRAME HANDLING UTILS */


#define START_DELIMITER (0b10011010)

typedef struct frame
{
    bool start_delimiter[8]; // Four bit start delimiter of 1001101
    bool data[8]; // Data bits
    bool parity;
} frame_t;

#define FRAME_BITLEN (17)


static inline uint8_t byte_from_bools(bool *bits)
{
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++)
    {
        byte |= (bits[i] << (7 - i));
    }
    return byte;
}

static inline void byte_to_bools(uint8_t byte, bool *bits)
{
    // printf("Byte: %d\n", byte);
    for (int i = 0; i < 8; i++)
    {
        bits[i] = (byte >> (7 - i)) & 1;
    }

    // Print the bits
    // printf("Bits: ");
    // for (int i = 0; i < 8; i++)
    // {
    //     printf("%d", bits[i]);
    // }
    // // printf("\n");
    // fflush(stdout);
}

static inline bool calculate_parity(bool *data, int size)
{
    bool parity = 0;
    for (int i = 0; i < size; i++)
    {
        parity ^= data[i]; // XOR all bits to calculate even parity
    }
    return parity;
}

static inline bool valid_frame(frame_t f)
{
    // Check if the start delimiter is valid
    uint8_t start_delimiter = byte_from_bools(f.start_delimiter);
    if (start_delimiter != START_DELIMITER)
    {
        return false;
    }

    // Check parity
    bool parity = calculate_parity(f.data, 8);
    return parity == f.parity;
}


static inline frame_t construct_frame(uint8_t data)
{
    frame_t f;
    memset(&f, 0, sizeof(frame_t));

    // Start delimiter (8 bits, 1101101)
    uint8_t start_delimiter = START_DELIMITER;
    byte_to_bools(start_delimiter, f.start_delimiter);

    // Set data
    byte_to_bools(data, f.data);

    // Calculate parity (even parity)
    bool parity = 0;
    for (int i = 0; i < 8; i++)
    {
        parity ^= ((data >> i) & 1);
    }

    f.parity = parity;

   // assert(valid_frame(f) == true);

    return f;
}

static inline void print_bools(bool *bits, int size)
{
    for (int i = 0; i < size; i++)
    {
        printf("%d", bits[i]);
        if (i == 3)
        {
            printf(" ");
        }

        // if (i % 32 == 31)
        // {
        //     printf("\n");
        // }
    }
}

static inline void print_frame(frame_t f)
{
    printf("Frame: | ");
    printf("  ");
    printf("Start Delim: 0x%02X", byte_from_bools(f.start_delimiter));
    printf(" |  ");
    printf("Data: 0x%02X (%c)", byte_from_bools(f.data), byte_from_bools(f.data));
    printf(" |  ");
    printf("Parity: %d (true parity=%d)", f.parity, calculate_parity(f.data, 8));
    printf("\n");
    printf("Bits: ");
    
    print_bools(f.start_delimiter, sizeof(f.start_delimiter));
    print_bools(f.data, sizeof(f.data));
    printf("%d", f.parity);

    printf("\n\n");
    fflush(stdout);
}


#endif // COVERT_UTILS_H