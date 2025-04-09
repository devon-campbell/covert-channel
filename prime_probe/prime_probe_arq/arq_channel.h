#ifndef ARQ_CHANNEL_H
#define ARQ_CHANNEL_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "covert_utils.h"
#include "phy.h"

#define ARQ_TIMEOUT (25 * 1000)  // microseconds

typedef struct {
    bool init_seq_num[6];
    frame_t frame;
    bool final_seq_num[5];
} arq_frame_t;

static inline bool calculate_seq_num(arq_frame_t *f) {
    int votes_zero = 0, votes_one = 0;
    for (int i = 0; i < 6; i++) votes_one += f->init_seq_num[i];
    for (int i = 0; i < 5; i++) votes_one += f->final_seq_num[i];
    votes_zero = 11 - votes_one;
    return votes_one > votes_zero;
}

static inline arq_frame_t construct_arq_frame(uint8_t data, bool seq_num) {
    arq_frame_t f = {0};
    for (int i = 0; i < 6; i++) f.init_seq_num[i] = seq_num;
    for (int i = 0; i < 5; i++) f.final_seq_num[i] = seq_num;
    f.frame = construct_frame(data);
    return f;
}

static inline void print_arq_frame(arq_frame_t f) {
    printf("------------------- ARQ Frame (Seq No %d) ------------------\n", calculate_seq_num(&f));
    print_frame(f.frame);
    printf("------------------- End of ARQ Frame ------------------------\n");
    fflush(stdout);
}

void transmit_arq_frame(uint8_t byte, bool seq_num);
arq_frame_t *receive_arq_frame(arq_frame_t *frame_buf, bool expected_seq_num);
void send_raw_arq_frame(arq_frame_t frame);
arq_frame_t *receive_raw_arq_frame(arq_frame_t *frame_buf, uint64_t timeout);

#endif  // ARQ_CHANNEL_H
