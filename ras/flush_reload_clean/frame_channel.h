#ifndef FRAME_CHANNEL_H
#define FRAME_CHANNEL_H

#include "covert_utils.h"
#include "send.h"
#include "receive.h"

// #define ARQ_TIMEOUT (6 * CHANNEL_SYNC_MAX * FRAME_BITLEN)
#define ARQ_TIMEOUT (25*1000)
typedef struct {
    bool init_seq_num[6];
    frame_t frame;
    bool final_seq_num[5];
} arq_frame_t;

static bool calculate_seq_num(arq_frame_t *f) {
    // Take majority vote to determine overall bit 
    int votes_zero = 0;
    int votes_one = 0;
    for (int i = 0; i < 6; i++) {
        if (f->init_seq_num[i]) {
            votes_one++;
        } else {
            votes_zero++;
        }
    }
    for (int i = 0; i < 5; i++) {
        if (f->final_seq_num[i]) {
            votes_one++;
        } else {
            votes_zero++;
        }
    }
    // Majority vote: if more than half are 1s, return 1, else return 0
    return (votes_one > votes_zero); 
}

static inline arq_frame_t construct_arq_frame(uint8_t data, bool seq_num) {
    arq_frame_t f;
    memset(&f, 0, sizeof(arq_frame_t));

    // Construct the frame
    // Set all initial sequence number bits to 0 or 1 based on seq_num
    for (int i = 0; i < 6; i++) {
        f.init_seq_num[i] = seq_num;
    }
    f.frame = construct_frame(data);

    // Set the final sequence number bits
    for (int i = 0; i < 5; i++) {
        f.final_seq_num[i] = seq_num;
    }

    // // Print the frame for debugging
    // printf("[construct_arq_frame] Constructed frame with data: %x, seq_num: %d\n", data, seq_num);
    
    return f;
}

static inline void print_arq_frame(arq_frame_t f) {
    printf("------------------- ARQ Frame (Seq No %d) ------------------\n", calculate_seq_num(&f));
    print_frame(f.frame);
    printf("------------------- End of ARQ Frame ------------------------\n");
    fflush(stdout);
}


void transmit_arq_frame(void *send_address, void *receive_address, uint8_t byte, bool seq_num);
arq_frame_t *receive_arq_frame(void *receive_address, void *send_address, arq_frame_t *frame_buf, bool expected_seq_num);

void send_raw_arq_frame(void *target_address, arq_frame_t frame);
arq_frame_t *receive_raw_arq_frame(void *target_address, arq_frame_t *frame_buf, uint64_t timeout);

// Address is sender address for client, receiver address for server
static inline void *get_client_address(){
    return get_event_address(0, NULL); // Default to first event address
}

// Address is sender address for server, receiver address for client
static inline void *get_server_address(){
    return get_event_address(0, "/bin/tmux"); 
}

#endif  // FRAME_CHANNEL_H