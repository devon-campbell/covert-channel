#define _GNU_SOURCE
#include "../bit_channel.h"
#include "../utils.h"
#include "pp_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <x86intrin.h>

volatile char *prime_probe_lines[SET_ASSOC];

/* ---------- one‑time init -------------------------------------------
    - Open shared memory region inside /dev/hugepages
    - Set fd size to BUFFER_SIZE
    - mmap the memory into vaddr space
    - memset to ensure pages are physically mapped
    - Find cache set matches
        - Iter through buffer, line-by-line (64 bytes each)
        - Get current candidate (buf + offset), isolate 6 LSB, check if matches TARGET_SET
            - If match, append to prime_probe_lines
*/
void pp_init(void)
{
    int fd = open("/dev/hugepages/pp_buf", O_CREAT | O_RDWR, 0666);
    if (fd < 0)
    {
        perror("open");
        exit(1);
    }
    if (ftruncate(fd, BUFFER_SIZE))
    {
        perror("ftruncate");
        exit(1);
    }

    void *buf = mmap(NULL, BUFFER_SIZE,
                     PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_HUGETLB,
                     fd, 0);
    if (buf == MAP_FAILED)
    {
        perror("mmap");
        exit(1);
    }

    memset(buf, 0, BUFFER_SIZE);

    int found = 0;
    for (size_t off = 0; off < BUFFER_SIZE && found < SET_ASSOC; off += LINE_SIZE)
    {
        void *cand = (char *)buf + off;
        if ((((uintptr_t)cand >> 6) & 0x3F) == TARGET_SET)
            prime_probe_lines[found++] = cand;
    }
    if (found < SET_ASSOC)
    {
        // fprintf(stderr, "[pp] need %d lines, found %d\n", SET_ASSOC, found);
        exit(1);
    }
    // fprintf(stderr, "[pp] set %d initialised (%d lines)\n", TARGET_SET, SET_ASSOC);
}

/* ---------- primitives --------------------------------------------- */
void prime_cache_set(void)
{
    for (int i = 0; i < SET_ASSOC; i++)
        (void)*prime_probe_lines[i];
    _mm_mfence();
}

void evict_cache_set(void)
{
    for (int r = 0; r < EVICT_ROUNDS; r++)
        for (int i = 0; i < SET_ASSOC; i++)
            _mm_clflush((void *)prime_probe_lines[i]);
    _mm_mfence();
}

uint64_t probe_cache_set(void)
{
    uint64_t tot = 0;
    for (int i = 0; i < SET_ASSOC; i++)
    {
        uint64_t t0 = rdtscp64();
        (void)*prime_probe_lines[i];
        tot += rdtscp64() - t0;
    }
    _mm_mfence();
    return tot;
}

static uint64_t thr;

/*
Initialize physical cahnnel
    - Map memory, find matching cache lines
    - Measure primed and evicted access times
    - Compute threshold
*/
void phy_init(void)
{
    pp_init();
    /* reuse good calibration: */
    const int R = 200;
    uint64_t p = 0, e = 0;
    for (int i = 0; i < R; i++)
    {
        prime_cache_set();
        p += probe_cache_set();
        prime_cache_set();
        evict_cache_set();
        e += probe_cache_set();
    }
    thr = p / R + (e / R - p / R) / 4; /* t0 + 1/4·gap  (safer) */
}

// Waits until start of new timing window (i.e. first few cycles)
static inline void wait_slot(void)
{
    while ((get_cycles32() & SLOT_MASK) > 1000)
        ;
}

// Waits until middle of bit window (i.e. when recvr should begin probing)
static inline void wait_half(void)
{
    while ((get_cycles32() & SLOT_MASK) < (SLOT_MASK >> 1))
        ;
}

/* -------- TX / RX --------------------------------------------------- */
static inline void phy_send_bit(bool bit)
{
    wait_slot();       /* slot start / synchronoize */
    prime_cache_set(); // fill cache w/ prime_probe_lines
    if (bit)
    {
        uint32_t half = SLOT_MASK >> 1;
        while ((get_cycles32() & SLOT_MASK) < half)
            evict_cache_set(); // evict while in first half of bit window
    }
    else
    {
        wait_half();
    }
}

static inline bool phy_recv_bit(void)
{
    wait_slot();
    prime_cache_set();
    wait_half();
    return probe_cache_set() > thr;
}

/* helpers */
void phy_send_bits(uint32_t v, int n)
{
    for (int i = n - 1; i >= 0; i--)
        phy_send_bit((v >> i) & 1);
}
uint32_t phy_recv_bits(int n)
{
    uint32_t v = 0;
    for (int i = 0; i < n; i++)
        v = (v << 1) | phy_recv_bit();
    return v;
}

void calibrate_bit_channel()
{
    phy_init();
}

inline void send_bit(void *target_address, bool bit)
{
    // Do nothing with target_address for now
    phy_send_bit(bit);
}
inline bool receive_bit(void *target_address)
{
    // Do nothing with target_address for now
    return phy_recv_bit();
}