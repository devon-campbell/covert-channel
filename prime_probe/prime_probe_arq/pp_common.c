/***************************  pp_common.c  *****************************/
#define _GNU_SOURCE
#include "pp_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>


volatile char *prime_probe_lines[SET_ASSOC];

/* ---------- one‑time init ------------------------------------------- */
void pp_init(void)
{
    int fd = open("/dev/hugepages/pp_buf", O_CREAT | O_RDWR, 0666);
    if (fd < 0) { perror("open"); exit(1); }
    if (ftruncate(fd, BUFFER_SIZE)) { perror("ftruncate"); exit(1); }

    void *buf = mmap(NULL, BUFFER_SIZE,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_HUGETLB,
                    fd, 0);
    if (buf == MAP_FAILED) { perror("mmap"); exit(1); }

    memset(buf, 0, BUFFER_SIZE);

    int found = 0;
    for (size_t off = 0; off < BUFFER_SIZE && found < SET_ASSOC; off += LINE_SIZE) {
        void *cand = (char*)buf + off;
        if ((((uintptr_t)cand >> 6) & 0x3F) == TARGET_SET)
            prime_probe_lines[found++] = cand;
    }
    if (found < SET_ASSOC) {
        fprintf(stderr, "[pp] need %d lines, found %d\n", SET_ASSOC, found);
        exit(1);
    }
    fprintf(stderr, "[pp] set %d initialised (%d lines)\n", TARGET_SET, SET_ASSOC);
}

/* ---------- primitives --------------------------------------------- */
void prime_cache_set(void)
{
    for (int i = 0; i < SET_ASSOC; i++) (void)*prime_probe_lines[i];
    _mm_mfence();
}

void evict_cache_set(void)
{
    for (int r = 0; r < EVICT_ROUNDS; r++)
        for (int i = 0; i < SET_ASSOC; i++)
            _mm_clflush((void*)prime_probe_lines[i]);
    _mm_mfence();
}

uint64_t probe_cache_set(void)
{
    uint64_t tot = 0;
    for (int i = 0; i < SET_ASSOC; i++) {
        uint64_t t0 = rdtscp64();
        (void)*prime_probe_lines[i];
        tot += rdtscp64() - t0;
    }
    _mm_mfence();
    return tot;
}
