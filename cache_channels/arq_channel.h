#ifndef ARQ_CHANNEL_H
#define ARQ_CHANNEL_H

// #define ARQ_TIMEOUT (6 * CHANNEL_SYNC_MAX * FRAME_BITLEN)
// #define ARQ_TIMEOUT (25*1000)
#include "arq_frame.h"

#include <stdint.h>
#include <stdbool.h>

void calibrate_frame_channel(void);
void transmit_arq_frame(void *send_address, void *receive_address, uint8_t byte, uint8_t seq_num, uint64_t timeout, int max_retries);
arq_frame_t *receive_arq_frame(void *in_addr, void *out_addr, arq_frame_t *frame_buf, uint8_t expected_seq_num);
#endif  // ARQ_CHANNEL_H