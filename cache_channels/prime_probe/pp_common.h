/***************************  pp_common.h  *****************************/
#ifndef PP_COMMON_H
#define PP_COMMON_H

#include <stdint.h>
#include <x86intrin.h>

/* ------------ global configuration ---------------------------------- */
#define TARGET_SET 3 /* cache set index we use (bits 6‑11) */
#define SET_ASSOC 16 /* lines per LLC set                  */
#define LINE_SIZE 64
#define BUFFER_SIZE (8 * 1024 * 1024UL) /* 8 MB buffer (multiple of 2 MB) */
#define SLOT_MASK 0x3FFFF               /* 18‑bit mask  → ≈87 µs @3 GHz        */
#define EVICT_ROUNDS 8                  /* how often to clflush per slot      */

/* ------------ globals filled by pp_init() --------------------------- */
extern volatile char *prime_probe_lines[SET_ASSOC];

/* ------------ helpers ---------------------------------------------- */
static inline uint64_t rdtscp64(void)
{
    unsigned aux;
    return __rdtscp(&aux);
}
static inline uint32_t get_cycles32(void)
{
    return (uint32_t)rdtscp64();
}

/* ------------ API --------------------------------------------------- */
void pp_init(void);
void prime_cache_set(void);
void evict_cache_set(void); /* uses clflush */
uint64_t probe_cache_set(void);

#endif /* PP_COMMON_H */