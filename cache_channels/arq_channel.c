#include "arq_channel.h"
#include "bit_channel.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

// Define info macro for always-on logging

void calibrate_frame_channel(void)
{
    calibrate_bit_channel();
}

void send_raw_frame(void *target_address, arq_frame_t frame)
{
    bool bool_buffer[ARQ_FRAME_BITLEN];
    debug_macro("[send_raw_frame] Flattening ARQ frame...\n");
    flatten_arq_frame(&frame, bool_buffer);
    debug_macro("[send_raw_frame] Sending flattened frame...\n");
    send_bits(bool_buffer, ARQ_FRAME_BITLEN, target_address);
    // send_bits(frame.init_seq_num, sizeof(frame.init_seq_num), target_address); // Send initial sequence number bits
    // send_bits(frame.frame.start_delimiter, sizeof(frame.frame.start_delimiter), target_address);
    // send_bits(frame.frame.data, sizeof(frame.frame.data), target_address);
    // send_bit(target_address, frame.frame.parity);
    // send_bit(frame.final_seq_num, sizeof(frame.final_seq_num)); // Send the sequence number bit
    debug_macro("[send_raw_frame] Sent flattened ARQ frame with seq_num: %d\n", frame_seq_num(&frame));
}

// Receive a byte frame: state machine that waits for the preamble, start delimiter, reads data and parity
arq_frame_t *receive_raw_frame(void *target_address, arq_frame_t *frame_buf, uint64_t timeout)
{
    memset(frame_buf, 0, sizeof(arq_frame_t));
    bool hist_window[6] = {0}; // History window for initial sequence number bits
    // In waiting state, look for the start delimiter with 8 bit shift register
    uint8_t shift_reg = 0;
    // start timeout time (microseconds)
    // uint64_t start_time = rdtscp64();
    uint64_t start_time_us = get_usec();

    // printf("[receive_byte_frame] Start time: %lu microseconds\n", get_usec());

    debug_macro("[receive_raw_frame] Waiting until %lx us for start delimiter...\n", timeout);
    while (1)
    {
        // Save the 6 bits preceding the shift register for initial sequence number

        // shift hist window, loosing oldest value (e.g. hist_window[5] = hist_window[4], 4 <- 3, 2 <- 1, 1 <- 0)
        for (int i = 5; i > 0; i--)
        {
            hist_window[i] = hist_window[i - 1]; // Shift left
        }
        // newewst hist window value from shift_reg
        hist_window[0] = (shift_reg >> 7) & 1; // Get the newest bit (the 8th bit of shift_reg)

        // Shift in the next bit
        shift_reg = (shift_reg << 1) | receive_bit(target_address);

        // fflush(stdout);
        // Check for start delimiter (0b10011010)
        if (shift_reg == START_DELIMITER)
        {

            break; // Found start delimiter
        }
        // Check for timeout
        // fflush(stdout);
        if (timeout > 0 && (get_usec() - start_time_us) > timeout)
        {
            debug_macro("Timeout waiting for start delimiter\n");
            // printf("[receive_byte_frame] Timeout time: %lu microseconds\n", get_usec());
            return NULL; // Timeout occurred
        }
        // if (timeout > 0 && (rdtscp64() - start_time) > timeout)
        // {
        //     printf("Timeout waiting for start delimiter\n");
        //     printf("[receive_byte_frame] Timeout time: %lu microseconds\n", get_usec());

        //     return NULL; // Timeout occurred
        // }
    }

    // Save the initial sequence number bits from the history window
    for (int i = 0; i < 6; i++)
    {
        frame_buf->seq_num_1[i] = hist_window[i];
    }

    // Save shift register as start delimiter
    byte_to_bools(shift_reg, frame_buf->frame.start_delimiter);

    // In receiving state, read the next 8 bits for data and 1 bit for parity
    for (int i = 0; i < 8; i++)
    {
        frame_buf->frame.data[i] = receive_bit(target_address);
    }
    frame_buf->frame.parity = receive_bit(target_address);
    // Save final sequence number bits
    for (int i = 0; i < 4; i++)
    {
        frame_buf->seq_num_2[i] = receive_bit(target_address);
    }
    for (int i = 0; i < 8; i++)
    {
        frame_buf->dup_data[i] = receive_bit(target_address);
    }
    for (int i = 0; i < 4; i++)
    {
        frame_buf->seq_num_3[i] = receive_bit(target_address);
    }
    for (int i = 0; i < 8; i++)
    {
        frame_buf->dup_data2[i] = receive_bit(target_address);
    }
    for (int i = 0; i < 4; i++)
    {
        frame_buf->seq_num_4[i] = receive_bit(target_address);
    }
    for (int i = 0; i < 8; i++)
    {
        frame_buf->dup_data3[i] = receive_bit(target_address);
    }

    // Now we allow anywhere from 0 to 16 1's, searching for reset delimeter
    uint8_t reset_shift_reg = 0;
    int count = 0;
    bool reset_delimiter_found = false;
    while (count < 24)
    {
        reset_shift_reg = (reset_shift_reg << 1) | receive_bit(target_address);
        count++;
        // Check for reset delimiter (0b11011010)
        if (reset_shift_reg == RESET_DELIMITER)
        {
            reset_delimiter_found = true;
            break; // Found reset delimiter
        }
    }

    if (!reset_delimiter_found)
    {
        debug_macro("[receive_byte_frame] Invalid frame, missing reset delimeter\n");
        // printf("[receive_byte_frame] Timeout time: %lu microseconds\n", get_usec());
        return NULL; // Timeout occurred
    }

    // Set reset delemeter in the frame buffer to all 1's
    for (int i = 0; i < 8; i++)
    {
        frame_buf->reset_delimiter[i] = 1;
    }

    // Here, we can now check data copies 4 and 5
    for (int i = 0; i < 8; i++)
    {
        frame_buf->dup_data4[i] = receive_bit(target_address);
    }
    for (int i = 0; i < 4; i++)
    {
        frame_buf->seq_num_5[i] = receive_bit(target_address);
    }
    for (int i = 0; i < 8; i++)
    {
        frame_buf->dup_data5[i] = receive_bit(target_address);
    }

    // Check frame validity
    if (!valid_frame(frame_buf))
    {
        info_macro("Invalid frame received, dumping it...\n");
#ifndef QUIET
        print_arq_frame(*frame_buf);
#endif
        return NULL;
    }
    info_macro("[receive_byte_frame] Valid frame received, dumping it...\n");
#ifndef QUIET
    print_arq_frame(*frame_buf);
#endif
    return frame_buf;
}

void transmit_arq_frame(void *send_address, void *receive_address, uint8_t byte, uint8_t seq_num, uint64_t timeout, int max_retries)
{
    arq_frame_t out_frame, ack_frame;
    memset(&out_frame, 0, sizeof(arq_frame_t));
    memset(&ack_frame, 0, sizeof(arq_frame_t));
    info_macro("--------------------Transmitting new ARQ packet (seq_num=%d, byte=%x)--------------------\n", seq_num, byte);

    int retries = 0;

    // Construct the frame to send
    out_frame = construct_arq_frame(byte, seq_num);

    // Wait for acknowledgment
    while (retries < max_retries)
    {
        retries++;
        debug_macro("[Transmit_arq_frame] Sending raw frame (seq_num=%d, byte=%x) (%d/%d)...\n", seq_num, byte, retries, max_retries);
#ifndef QUIET
        print_arq_frame(out_frame);
#endif
        send_raw_frame(send_address, out_frame);
        debug_macro("[Transmit_arq_frame] Frame sent, waiting for ACK (%lx cycles) (%d/%d)...\n", timeout, retries, max_retries);

        // Await the ACK packet, which has three results:
        // 1. ACK received with correct sequence number -> return and move on to next packet
        // 2. ACK timed out -> resend the frame
        // 3. Stale ACK received -> continue waiting for the correct ACK
        while (1)
        {
            debug_macro("[Transmit_arq_frame] Waiting for ACK...\n");
            arq_frame_t *received_frame = receive_raw_frame(receive_address, &ack_frame, timeout);
            if (received_frame == NULL)
            {
                // ACK timed out, so do retransmission
                debug_macro("[Transmit_arq_frame] Receive ACK failed (bad packet or timeout), retransmitting packet...\n");
                break;
            }
            // Some kind of an ACK received
            // TODO could retransmit if byte mismatch occurs
            uint8_t received_byte = frame_data(received_frame);
            uint8_t received_seq_num = frame_seq_num(received_frame);
            debug_macro("[Transmit_arq_frame] ACK received (seq_num=%d, byte=%x)\n", received_seq_num, received_byte);

            // If received ACK has correct sequence number, this packet was correctly transmitted so return
            if (received_seq_num == seq_num)
            {
                // printf("[Transmit_arq_frame] ACK confirmed for seq_num %d, moving on to next packet\n", seq_num);
                info_macro("--------------------ACK confirmed, moving on to next packet (seq_num=%d, recvd_byte=%x)--------------------\n", seq_num, received_byte);
                return; // Successfully acknowledged
            }
            else
            {
                // Likely a stale / bad ACK, continue waiting (but don't resend the frame)
                debug_macro("[Transmit_arq_frame] Stale ACK, seq_num mismatch (expected: %d, received: %d), continuing to wait for non-stale ACK...\n", seq_num, received_seq_num);
                // If ACK sequence number does not match, resend the frame
                continue;
            }
        }
    }
}

// Receive on the receive address and send ACK back to the sender, returns pointer to received frame (frame buffer)
arq_frame_t *receive_arq_frame(void *in_addr, void *out_addr, arq_frame_t *frame_buf, uint8_t expected_seq_num)
{
    memset(frame_buf, 0, sizeof(arq_frame_t));
    info_macro("--------------------[receive_arq_frame] Waiting for next frame (expect seq_num=%d)------------------\n", expected_seq_num);
    while (1)
    {
        debug_macro("[receive_arq_frame] Waiting for frame...\n");
        arq_frame_t *received_frame = receive_raw_frame(in_addr, frame_buf, 0);
        if (received_frame == NULL)
        {
            // For now, just drop)
            info_macro("[receive_arq_frame] BAD FRAME: Got invalid frame, dropping without ACK and waiting on retransmission...\n");
            continue; // Retry receiving
        }

        uint8_t byte = frame_data(received_frame);
        uint8_t received_seq_num = frame_seq_num(received_frame);
        debug_macro("[receive_arq_frame] Received frame: (seq_num=%d, byte=%x)\n", received_seq_num, byte);

        // Check if the sequence number matches the expected one
        if (received_seq_num != expected_seq_num)
        {
            debug_macro("[Transmit_arq_frame] Stale packet, seq_num mismatch (expected: %d, received: %d), retransmitting ACK\n", expected_seq_num, received_seq_num);
            arq_frame_t ack_frame = construct_arq_frame(byte, received_seq_num);
            send_raw_frame(out_addr, ack_frame);
            continue; // Retry receiving
        }

        // Send ACK for the received byte
        arq_frame_t ack_frame = construct_arq_frame(byte, received_seq_num);
        send_raw_frame(out_addr, ack_frame); // Acknowledge the received byte

        info_macro("---------------------[receive_arq_frame] Valid frame received and ACK'd, moving on (seq_num=%d, byte=%x)------------------\n", received_seq_num, byte);

        return received_frame; // Return the valid frame
    }
}