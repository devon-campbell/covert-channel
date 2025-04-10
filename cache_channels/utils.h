#ifndef UTILS_H
#define UTILS_H
#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>

#include "log.h"

# define FORCE_INLINE __attribute__((always_inline)) inline

static FORCE_INLINE uint8_t byte_from_bools(bool *bits)
{
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++)
    {
        byte |= (bits[i] << (7 - i));
    }
    return byte;
}

static FORCE_INLINE void byte_to_bools(uint8_t byte, bool *bits)
{
    // printf("Byte: %d\n", byte);
    for (int i = 0; i < 8; i++)
    {
        bits[i] = (byte >> (7 - i)) & 1;
    }
}
static FORCE_INLINE void byte_to_two_bit(uint8_t byte, bool *bits)
{
    // debug(stderr, "Byte: %d\n", byte);
    for (int i = 0; i < 2; i++)
    {
        bits[i] = (byte >> (1 - i)) & 1;
    }
}

static FORCE_INLINE uint8_t two_bit_to_byte(bool *bits)
{
    uint8_t byte = 0;
    for (int i = 0; i < 2; i++)
    {
        byte |= (bits[i] << (1 - i));
    }

    return byte;
}



#define CRC_START_8 0xFF
static uint8_t sht75_crc_table[] = {

	0,   49,  98,  83,  196, 245, 166, 151, 185, 136, 219, 234, 125, 76,  31,  46,
	67,  114, 33,  16,  135, 182, 229, 212, 250, 203, 152, 169, 62,  15,  92,  109,
	134, 183, 228, 213, 66,  115, 32,  17,  63,  14,  93,  108, 251, 202, 153, 168,
	197, 244, 167, 150, 1,   48,  99,  82,  124, 77,  30,  47,  184, 137, 218, 235,
	61,  12,  95,  110, 249, 200, 155, 170, 132, 181, 230, 215, 64,  113, 34,  19,
	126, 79,  28,  45,  186, 139, 216, 233, 199, 246, 165, 148, 3,   50,  97,  80,
	187, 138, 217, 232, 127, 78,  29,  44,  2,   51,  96,  81,  198, 247, 164, 149,
	248, 201, 154, 171, 60,  13,  94,  111, 65,  112, 35,  18,  133, 180, 231, 214,
	122, 75,  24,  41,  190, 143, 220, 237, 195, 242, 161, 144, 7,   54,  101, 84,
	57,  8,   91,  106, 253, 204, 159, 174, 128, 177, 226, 211, 68,  117, 38,  23,
	252, 205, 158, 175, 56,  9,   90,  107, 69,  116, 39,  22,  129, 176, 227, 210,
	191, 142, 221, 236, 123, 74,  25,  40,  6,   55,  100, 85,  194, 243, 160, 145,
	71,  118, 37,  20,  131, 178, 225, 208, 254, 207, 156, 173, 58,  11,  88,  105,
	4,   53,  102, 87,  192, 241, 162, 147, 189, 140, 223, 238, 121, 72,  27,  42,
	193, 240, 163, 146, 5,   52,  103, 86,  120, 73,  26,  43,  188, 141, 222, 239,
	130, 179, 224, 209, 70,  119, 36,  21,  59,  10,  89,  104, 255, 206, 157, 172
};

/*
 * uint8_t crc_8( const unsigned char *input_str, size_t num_bytes );
 *
 * The function crc_8() calculates the 8 bit wide CRC of an input string of a
 * given length.
 */

static FORCE_INLINE uint8_t crc_8( const uint8_t *bytes, size_t num ) {

	size_t a;
	uint8_t crc;
	const unsigned char *ptr;

	crc = CRC_START_8;
	ptr = bytes;

	if ( ptr != NULL ) for (a=0; a<num; a++) {

		crc = sht75_crc_table[(*ptr++) ^ crc];
	}

	return crc;

}  /* crc_8 */

static FORCE_INLINE uint8_t crc_8_byte( uint8_t byte ) {

    return crc_8(&byte, 1);
}  /* crc_8_byte */

static FORCE_INLINE bool calculate_parity(bool *data, int size)
{
    bool parity = 0;
    for (int i = 0; i < size; i++)
    {
        parity ^= data[i]; // XOR all bits to calculate even parity
    }
    return parity;
}

static FORCE_INLINE uint64_t get_usec()
{
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    return current_time.tv_sec * 1000000 + current_time.tv_nsec / 1000;
}

#define FILE_MAP_SIZE 4096
static FORCE_INLINE void *shared_file_address(const char *path, size_t offset)
{
    int fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        perror("[shared_file_address] Failed to open file");
        return NULL;
    }

    void *base = mmap(NULL, FILE_MAP_SIZE, PROT_READ, MAP_SHARED, fd, 0);
    if (base == MAP_FAILED)
    {
        perror("[shared_file_address] Failed to map address");
        close(fd);
        return NULL;
    }

    void *addr = (void *)((uint64_t)base + offset);

    return addr;
}

// #include <stdarg.h>
// // Log levels
// #define LOG_DEBUG 0
// #define LOG_INFO 1
// #define LOG_ERROR 2

// #define LOGLEVEL LOG_DEBUG
// #define QUIET

// static FILE *log_fd = NULL;
// static FORCE_INLINE void set_log_fd(FILE *fd)
// {
//     log_fd = stderr;
// }
// static FORCE_INLINE FILE *get_log_fd()
// {
//     // If ite
//     return log_fd;
// }

// static FORCE_INLINE int chan_log(int level, const char *format, ...)
// {
// #ifndef QUIET
//     if (log_fd == NULL)
//     {
//         log_fd = stderr; // Default to stderr if log_fd is not set
//     }
//     if (level >= LOGLEVEL) {
//         va_list args;
//         int result;
        
//         va_start(args, format);
//         result = vfprintf(log_fd, format, args);
//         va_end(args);
        
//         return result;
//     }
// #endif
//     (void)format; // Suppress unused parameter warning
//     return 0;
// }

// static FORCE_INLINE int debug(const char *format, ...)
// {
// #ifndef QUIET
//     if (log_fd == NULL)
//     {
//         log_fd = stderr; // Default to stderr if log_fd is not set
//     }
//     if (LOG_DEBUG >= LOGLEVEL) {
//         va_list args;
//         int result;
        
//         va_start(args, format);
//         fprintf(log_fd, "[DEBUG] ");
//         result = vfprintf(log_fd, format, args);
//         va_end(args);
        
//         return result;
//     }
// #endif
//     (void)format; // Suppress unused parameter warning
//     return 0;
// }

// static FORCE_INLINE int info(const char *format, ...)
// {
// #ifndef QUIET
//     if (log_fd == NULL)
//     {
//         log_fd = stderr; // Default to stderr if log_fd is not set
//     }
//     if (LOG_INFO >= LOGLEVEL) {
//         va_list args;
//         int result;
        
//         va_start(args, format);
//         fprintf(log_fd, "[INFO] ");
//         result = vfprintf(log_fd, format, args);
//         va_end(args);
        
//         return result;
//     }
// #endif
//     (void)format; // Suppress unused parameter warning
//     return 0;
// }

// static FORCE_INLINE int error(const char *format, ...)
// {
// #ifndef QUIET
//     if (log_fd == NULL)
//     {
//         log_fd = stderr; // Default to stderr if log_fd is not set
//     }
//     if (LOG_ERROR >= LOGLEVEL) {
//         va_list args;
//         int result;
        
//         va_start(args, format);
//         fprintf(log_fd, "[ERROR] ");
//         result = vfprintf(log_fd, format, args);
//         va_end(args);
        
//         return result;
//     }
// #endif
//     (void)format; // Suppress unused parameter warning
//     return 0;
// }

// static FORCE_INLINE int debug_np(const char *format, ...)
// {
// #ifndef QUIET
//     if (log_fd == NULL)
//     {
//         log_fd = stderr; // Default to stderr if log_fd is not set
//     }
//     if (LOG_DEBUG >= LOGLEVEL) {
//         va_list args;
//         int result;
        
//         va_start(args, format);
//         result = vfprintf(log_fd, format, args);
//         va_end(args);
        
//         return result;
//     }
// #endif
//     (void)format; // Suppress unused parameter warning
//     return 0;
// }

// static FORCE_INLINE void print_bools(bool *bits, int size)
// {
//     for (int i = 0; i < size; i++)
//     {
//         chan_log(LOG_DEBUG, "%d", bits[i]);
//         if ((i + 1) % 4 == 0 && i < size - 1)
//         {
//             chan_log(LOG_DEBUG, " ");
//         }
//     }
// }

#endif // UTILS_H