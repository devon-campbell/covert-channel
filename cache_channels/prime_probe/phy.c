#include "phy.h"
#include "pp_common.h"   /*  working P+P */
#include <x86intrin.h>

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
    const int R=200; uint64_t p=0,e=0;
    for(int i=0;i<R;i++){ prime_cache_set(); p+=probe_cache_set();
                          prime_cache_set(); evict_cache_set(); e+=probe_cache_set();}
    thr = p/R + (e/R - p/R)/4;          /* t0 + 1/4·gap  (safer) */
}

// Waits until start of new timing window (i.e. first few cycles)
static inline void wait_slot(void)
{
    while((get_cycles32() & SLOT_MASK) >  1000);
}

// Waits until middle of bit window (i.e. when recvr should begin probing)
static inline void wait_half(void)
{
    while((get_cycles32() & SLOT_MASK) < (SLOT_MASK>>1));
}

/* -------- TX / RX --------------------------------------------------- */
void phy_send_bit(bool bit)
{
    wait_slot();                /* slot start / synchronoize */
    prime_cache_set();          // fill cache w/ prime_probe_lines
    if(bit){
        uint32_t half=SLOT_MASK>>1;
        while((get_cycles32() & SLOT_MASK)<half) evict_cache_set(); // evict while in first half of bit window
    }else{
        wait_half();
    }
}

bool phy_recv_bit(void)
{
    wait_slot();
    prime_cache_set();
    wait_half();
    return probe_cache_set() > thr;
}

/* helpers */
void phy_send_bits(uint32_t v,int n){ for(int i=n-1;i>=0;i--) phy_send_bit((v>>i)&1); }
uint32_t phy_recv_bits(int n){ uint32_t v=0; for(int i=0;i<n;i++) v=(v<<1)|phy_recv_bit(); return v; }
