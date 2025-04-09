#define _POSIX_C_SOURCE 200809L

#include "arq_channel.h"
#include "phy.h"
#include "covert_utils.h"
#include <time.h>
#include <stdio.h>
#include <string.h>

static inline uint64_t get_usec()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64_t)ts.tv_sec * 1000000ULL) + (ts.tv_nsec / 1000ULL);
}

uint32_t bits_to_u32(const bool *bits, int n) {
    uint32_t out = 0;
    for (int i = 0; i < n; i++) {
        out <<= 1;
        out |= bits[i];
    }
    return out;
}


void send_raw_arq_frame(arq_frame_t frame)
{
    phy_send_bits(bits_to_u32(frame.init_seq_num, 6), 6);
    phy_send_bits(byte_from_bools(frame.frame.start_delimiter), 8);
    phy_send_bits(byte_from_bools(frame.frame.data), 8);
    phy_send_bit(frame.frame.parity);
    phy_send_bits(bits_to_u32(frame.final_seq_num, 5), 5);

    //printf("[send_raw_arq_frame] Sent ARQ frame with seq_num=%d\n", calculate_seq_num(&frame));
}

arq_frame_t *receive_raw_arq_frame(arq_frame_t *frame_buf, uint64_t timeout)
{
    memset(frame_buf, 0, sizeof(arq_frame_t));

    bool hist_window[6] = {0};
    uint8_t shift_reg = 0;
    uint64_t start_time_us = get_usec();

    //printf("[receive_raw_arq_frame] Waiting up to %lu us for start delimiter...\n", timeout);

    while (1) {
        for (int i = 5; i > 0; i--) hist_window[i] = hist_window[i - 1];
        hist_window[0] = (shift_reg >> 7) & 1;

        bool bit = phy_recv_bit();
        shift_reg = (shift_reg << 1) | (bit ? 1 : 0);

        if (shift_reg == START_DELIMITER) break;
        if (timeout && (get_usec() - start_time_us) > timeout) {
            //printf("Timeout waiting for start delimiter\n");
            return NULL;
        }
    }

    for (int i = 0; i < 6; i++) frame_buf->init_seq_num[i] = hist_window[i];
    byte_to_bools(shift_reg, frame_buf->frame.start_delimiter);

    for (int i = 0; i < 8; i++) frame_buf->frame.data[i] = phy_recv_bit();
    frame_buf->frame.parity = phy_recv_bit();
    for (int i = 0; i < 5; i++) frame_buf->final_seq_num[i] = phy_recv_bit();

    if (!valid_frame(frame_buf->frame)) {
        //printf("Invalid frame received\n");
        // print_arq_frame(*frame_buf);
        return NULL;
    }

    // printf("[receive_raw_arq_frame] Valid frame received\n");
    return frame_buf;
}

void transmit_arq_frame(uint8_t byte, bool seq_num)
{
    // printf("--------- Transmitting ARQ packet (seq_num=%d, byte=0x%x) ---------\n",
    //        seq_num, byte);

    arq_frame_t out_frame = construct_arq_frame(byte, seq_num);
    arq_frame_t ack_frame = {0};
    int max_retries = 100, retries = 0;

    while (retries++ < max_retries) {
        send_raw_arq_frame(out_frame);
        // printf("[transmit_arq_frame] Frame sent, waiting for ACK (attempt %d/%d)\n", retries, max_retries);

        while (1) {
            arq_frame_t *received_frame = receive_raw_arq_frame(&ack_frame, ARQ_TIMEOUT);
            if (!received_frame) {
                // printf("[transmit_arq_frame] ACK TIMEOUT, retransmitting...\n");
                break;
            }

            // uint8_t ack_byte = byte_from_bools(received_frame->frame.data);
            bool ack_seq = calculate_seq_num(received_frame);
            // printf("[transmit_arq_frame] ACK received (seq_num=%d, byte=0x%x)\n", ack_seq, ack_byte);

            if (ack_seq == seq_num) {
                // printf("------- ACK confirmed, moving on (seq_num=%d) -------\n", seq_num);
                return;
            } else {
                // printf("[transmit_arq_frame] Stale ACK (expected %d, got %d), waiting...\n", seq_num, ack_seq);
            }
        }
    }

    // printf("[transmit_arq_frame] Gave up after %d retries.\n", max_retries);
}

arq_frame_t *receive_arq_frame(arq_frame_t *frame_buf, bool expected_seq_num)
{
    memset(frame_buf, 0, sizeof(arq_frame_t));
    // printf("------ [receive_arq_frame] Waiting (expect seq_num=%d) ------\n", expected_seq_num);

    while (1) {
        arq_frame_t *received_frame = receive_raw_arq_frame(frame_buf, ARQ_TIMEOUT);
        if (!received_frame) {
            // printf("[receive_arq_frame] BAD FRAME or TIMEOUT => retrying...\n");
            continue;
        }

        uint8_t data_byte = byte_from_bools(received_frame->frame.data);
        bool received_seq = calculate_seq_num(received_frame);
        // printf("[receive_arq_frame] Got frame: seq_num=%d, byte=0x%x\n", received_seq, data_byte);

        if (received_seq != expected_seq_num) {
            // printf("[receive_arq_frame] Stale packet => re-ACK\n");
            arq_frame_t ack = construct_arq_frame(data_byte, received_seq);
            send_raw_arq_frame(ack);
            continue;
        }

        arq_frame_t ack = construct_arq_frame(data_byte, received_seq);
        send_raw_arq_frame(ack);
        // printf("[receive_arq_frame] ACK sent => returning frame\n");
        return received_frame;
    }
}
