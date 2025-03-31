#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <x86intrin.h>

/***********************************************************
 * Configuration
 ***********************************************************/
#define TARGET_SET     3
#define LLC_NUM_SETS   64   // bits 6..11 => 0..63
#define SET_ASSOC      16
#define BUFFER_SIZE    (8 * 1024 * 1024) // 8 MB buffer for more addresses
#define PAGE_SIZE      4096
#define LINE_SIZE      64

static int pagemap_fd = -1;

/***********************************************************
 * open /proc/self/pagemap
 ***********************************************************/
int open_pagemap(void) {
    if (pagemap_fd == -1) {
        pagemap_fd = open("/proc/self/pagemap", O_RDONLY);
        if (pagemap_fd < 0) {
            perror("open /proc/self/pagemap");
            return -1;
        }
    }
    return pagemap_fd;
}

/***********************************************************
 * read physical addr
 ***********************************************************/
uint64_t get_physical_address(void *vaddr) {
    if (open_pagemap() < 0) {
        return 0;
    }

    uint64_t vpn = (uint64_t)vaddr >> 12;
    off_t offset = vpn * sizeof(uint64_t);

    if (lseek(pagemap_fd, offset, SEEK_SET) == (off_t)-1) {
        perror("lseek");
        return 0;
    }

    uint64_t entry;
    if (read(pagemap_fd, &entry, 8) != 8) {
        perror("read pagemap");
        return 0;
    }

    // Check if page is present
    if ((entry & ((uint64_t)1 << 63)) == 0) {
        return 0;
    }

    uint64_t pfn = entry & 0x7FFFFFFFFFFFFFULL;
    uint64_t page_off = (uint64_t)vaddr & (PAGE_SIZE - 1);

    return (pfn << 12) | page_off;
}

/***********************************************************
 * read timestamp
 ***********************************************************/
static inline uint64_t rdtscp64() {
    unsigned aux;
    return __rdtscp(&aux);
}

/***********************************************************
 * measure_access_time: Access an address, measure how many cycles it took
 ***********************************************************/
static inline uint64_t measure_access_time(volatile char *addr) {
    uint64_t start = rdtscp64();
    *addr;
    uint64_t end = rdtscp64();
    return end - start;
}

/***********************************************************
 * prime_cache_set: Access all addresses once to load them into the cache
 ***********************************************************/
void prime_cache_set(volatile char **lines, int count) {
    for (int i = 0; i < count; i++) {
        *lines[i];
    }
    _mm_mfence();
}

/***********************************************************
 * probe_cache_set: Re-access all addresses, sum total time
 ***********************************************************/
uint64_t probe_cache_set(volatile char **lines, int count) {
    uint64_t total = 0;
    for (int i = 0; i < count; i++) {
        total += measure_access_time(lines[i]);
    }
    _mm_mfence();
    return total;
}

/***********************************************************
 * main
 *  - Gathers 2*SET_ASSOC addresses for the set
 *  - 16 prime/probe, 16 eviction
 *  - Repeats measurement multiple times (rounds=500)
 *  - eviction loop with many writes
 ***********************************************************/
int main() {
    // 1) Allocate buffer
    void *big_buffer = aligned_alloc(PAGE_SIZE, BUFFER_SIZE);
    if (!big_buffer) {
        perror("aligned_alloc");
        return 1;
    }
    memset(big_buffer, 0, BUFFER_SIZE);

    // 2) Gather addresses
    volatile char *prime_probe_lines[SET_ASSOC];
    volatile char *evict_lines[SET_ASSOC];
    int found_count = 0;

    for (size_t offset = 0; offset < BUFFER_SIZE; offset += LINE_SIZE) {
        void *candidate = (char *)big_buffer + offset;
        uint64_t phys = get_physical_address(candidate);
        if (!phys) continue;

        uint64_t set_index = (phys >> 6) & 0x3F; // 0..63
        if (set_index == TARGET_SET) {
            if (found_count < SET_ASSOC) {
                prime_probe_lines[found_count] = (volatile char *)candidate;
            } else if (found_count < 2 * SET_ASSOC) {
                evict_lines[found_count - SET_ASSOC] = (volatile char *)candidate;
            }
            found_count++;
            if (found_count >= 2 * SET_ASSOC) break;
        }
    }

    if (found_count < 2 * SET_ASSOC) {
        printf("Could not find enough addresses for set %d (found %d)\n",
               TARGET_SET, found_count);
        close(pagemap_fd);
        free(big_buffer);
        return 1;
    }

    printf("Found %d addresses for set %d (16 for prime-probe, 16 for eviction)\n",
           found_count, TARGET_SET);

    // 3) We’ll measure multiple times
    int rounds = 500; // larger sampling
    uint64_t sum_time_no_evict = 0;
    uint64_t sum_time_evicted = 0;

    // 4) First scenario: NO eviction
    for (int r = 0; r < rounds; r++) {
        // prime
        prime_cache_set(prime_probe_lines, SET_ASSOC);
        // measure
        sum_time_no_evict += probe_cache_set(prime_probe_lines, SET_ASSOC);
    }
    uint64_t avg_no_evict = sum_time_no_evict / rounds;

    // 5) Second scenario: WITH eviction
    for (int r = 0; r < rounds; r++) {
        // prime
        prime_cache_set(prime_probe_lines, SET_ASSOC);

        // intense eviction: 
        // do writes to each address in evict_lines for many iterations
        for (int i = 0; i < 30000; i++) { // more rounds
            for (int j = 0; j < SET_ASSOC; j++) {
                evict_lines[j][0] = (char) j; // do a write instead of read
            }
            _mm_mfence();
        }

        // measure
        sum_time_evicted += probe_cache_set(prime_probe_lines, SET_ASSOC);
    }
    uint64_t avg_evicted = sum_time_evicted / rounds;

    printf("Average probe time WITHOUT eviction: %lu cycles\n", avg_no_evict);
    printf("Average probe time WITH    eviction: %lu cycles\n", avg_evicted);

    if (avg_evicted > avg_no_evict + 150) {
        printf("Detected measurable eviction effect! \n");
    } else {
        printf("Eviction effect is weak. Consider tuning further.\n");
    }

    close(pagemap_fd);
    free(big_buffer);
    return 0;
}
