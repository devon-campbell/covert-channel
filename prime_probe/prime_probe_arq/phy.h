/* phy.h  – Prime+Probe physical layer */
#pragma once
#include <stdbool.h>
#include <stdint.h>

void  phy_init(void);          /* one‑time init (pp_init + threshold)   */
void  phy_send_bit(bool b);    /* blocking: occupies exactly one slot   */
bool  phy_recv_bit(void);      /* blocking: returns next bit in slot    */
void  phy_send_bits(uint32_t val, int n);   /* helpers…                 */
uint32_t phy_recv_bits(int n);
