#include "phy.h"
#include "pp_common.h"   /*  working P+P */
#include <x86intrin.h>

static uint64_t thr;

void phy_init(void)
{
    pp_init();
    /* reuse good calibration: */
    const int R=200; uint64_t p=0,e=0;
    for(int i=0;i<R;i++){ prime_cache_set(); p+=probe_cache_set();
                          prime_cache_set(); evict_cache_set(); e+=probe_cache_set();}
    thr = p/R + (e/R - p/R)/4;          /* t0 + ¼·gap  (safer) */
}

static inline void wait_slot(void)
{
    while((get_cycles32() & SLOT_MASK) >  1000);
}

static inline void wait_half(void)
{
    while((get_cycles32() & SLOT_MASK) < (SLOT_MASK>>1));
}

/* -------- TX / RX --------------------------------------------------- */
void phy_send_bit(bool bit)
{
    wait_slot();                /* slot start */
    prime_cache_set();
    if(bit){
        uint32_t half=SLOT_MASK>>1;
        while((get_cycles32() & SLOT_MASK)<half) evict_cache_set();
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
