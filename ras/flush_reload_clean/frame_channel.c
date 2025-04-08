// Bidirectional frame channel that uses ARQ to make reliable communication
#define _POSIX_C_SOURCE 200809L

#include "send.h"
#include "receive.h"
#include "frame_channel.h"
#include <time.h>


static inline uint64_t get_usec()
{
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    return current_time.tv_sec * 1000000 + current_time.tv_nsec / 1000;
}

void send_raw_arq_frame(void *target_address, arq_frame_t frame)
{
    send_bits(frame.init_seq_num, sizeof(frame.init_seq_num), target_address); // Send initial sequence number bits
    send_bits(frame.frame.start_delimiter, sizeof(frame.frame.start_delimiter), target_address);
    send_bits(frame.frame.data, sizeof(frame.frame.data), target_address);
    send_bit(target_address, frame.frame.parity);
    send_bit(frame.final_seq_num, sizeof(frame.final_seq_num)); // Send the sequence number bit
    printf("[send_raw_arq_frame] Sent ARQ frame with seq_num: %d\n", calculate_seq_num(&frame));
    // print_arq_frame(frame);
}

// Receive a byte frame: state machine that waits for the preamble, start delimiter, reads data and parity
inline arq_frame_t *receive_raw_arq_frame(void *target_address, arq_frame_t *frame_buf, uint64_t timeout)
{
    memset(frame_buf, 0, sizeof(arq_frame_t));
    bool hist_window[6] = {0}; // History window for initial sequence number bits
    // In waiting state, look for the start delimiter with 8 bit shift register
    uint8_t shift_reg = 0;
    // start timeout time (microseconds)
    // uint64_t start_time = rdtscp64();
    uint64_t start_time_us = get_usec();

    // printf("[receive_byte_frame] Start time: %lu microseconds\n", get_usec());

    printf("[receive_byte_frame] Waiting until %lx us for start delimiter...\n", timeout);
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
        if ( timeout > 0 && (get_usec() - start_time_us) > timeout)
        {
            printf("Timeout waiting for start delimiter\n");
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
        frame_buf->init_seq_num[i] = hist_window[i];
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
    for (int i = 0; i < 5; i++)
    {
        frame_buf->final_seq_num[i] = receive_bit(target_address);
    }

    // Check frame validity
    if (!valid_frame(frame_buf->frame))
    {
        printf("Invalid frame received\n");
        print_arq_frame(*frame_buf);
        return NULL;
    }
    printf("[receive_byte_frame] Valid frame received\n");
   // print_arq_frame(*frame_buf);
    return frame_buf;
}

// Send to send address, wait for ACK from receive address
void transmit_arq_frame(void *send_address, void *receive_address, uint8_t byte, bool seq_num)
{
    arq_frame_t out_frame, ack_frame;
    memset(&out_frame, 0, sizeof(arq_frame_t));
    memset(&ack_frame, 0, sizeof(arq_frame_t));
    printf("--------------------Transmitting new ARQ packet (seq_num=%d, byte=%x)--------------------\n", seq_num, byte);
    // printf("[Transmit_arq_frame] [CORRECT PATH] NEW PACKET: Transmitting byte: %x with seq number %d\n", byte, seq_num);

    uint64_t timeout = ARQ_TIMEOUT;
    int max_retries = 100; // Maximum number of retries for sending the frame
    int retries = 0;

    // Construct the frame to send
    out_frame = construct_arq_frame(byte, seq_num);

    // Wait for acknowledgment
    while (retries < max_retries)
    {
        retries++;
        // usleep(1000*500);

        // Send the frame
        send_raw_arq_frame(send_address, out_frame);
        printf("[Transmit_arq_frame] Frame sent, waiting for ACK (%lx cycles) (%d/%d)...\n", timeout, retries, max_retries);
        fflush(stdout);

        // Await the ACK packet, which has three results:
        // 1. ACK received with correct sequence number -> return and move on to next packet
        // 2. ACK timed out -> resend the frame
        // 3. Stale ACK received -> continue waiting for the correct ACK
        while (1)
        {
            arq_frame_t *received_frame = receive_raw_arq_frame(receive_address, &ack_frame, timeout);
            if (received_frame == NULL)
            {  
                // ACK timed out, so do retransmission
                printf("[Transmit_arq_frame] ACK TIME OUT, retransmitting packet...\n");
                break;
            }
            // Some kind of an ACK received
            // TODO could retransmit if byte mismatch occurs
            uint8_t received_byte = byte_from_bools(received_frame->frame.data);
            bool received_seq_num = calculate_seq_num(received_frame);
            printf("[Transmit_arq_frame] ACK received (seq_num=%d, byte=%x)\n", received_seq_num, received_byte);

            // If received ACK has correct sequence number, this packet was correctly transmitted so return
            if (received_seq_num == seq_num)
            {
                // printf("[Transmit_arq_frame] ACK confirmed for seq_num %d, moving on to next packet\n", seq_num);
                printf("--------------------ACK confirmed, moving on to next packet (seq_num=%d, recvd_byte=%x)--------------------\n", seq_num, received_byte);
                return; // Successfully acknowledged
            }
            else
            {
                // Likely a stale / bad ACK, continue waiting (but don't resend the frame)
                printf("[Transmit_arq_frame] Stale ACK, seq_num mismatch (expected: %d, received: %d), continuing to wait for non-stale ACK...\n", seq_num, received_seq_num);
                // If ACK sequence number does not match, resend the frame
                continue;
            }
        }
    }
}

// Receive on the receive address and send ACK back to the sender
arq_frame_t *receive_arq_frame(void *receive_address, void *send_address, arq_frame_t *frame_buf, bool expected_seq_num)
{
    memset(frame_buf, 0, sizeof(arq_frame_t));
    // Initially, we expect the sequence number to be 0
    printf("--------------------[receive_arq_frame] Waiting for next frame (expect seq_num=%d)------------------\n", expected_seq_num);
    // printf("[receive_arq_frame] [CORRECT PATH] Waiting for frame with expected seq_num: %d\n", expected_seq_num);
    while (1)
    {
        // Receive a byte frame
        printf("[receive_arq_frame] Waiting for frame...\n");
        arq_frame_t *received_frame = receive_raw_arq_frame(receive_address, frame_buf, 0);
        if (received_frame == NULL)
        {
            // For now, just drop
            printf("[receive_arq_frame] BAD FRAME: Got invalid frame, dropping without ACK and waiting on retransmission...\n");
            // Send a NAK (frame with 0 bytes, 0 seq num) to indicate failure
            // arq_frame_t nak_frame = construct_arq_frame(0, 0);
            // send_raw_arq_frame(send_address, nak_frame);
            // printf("[receive_arq_frame] Sent NAK for invalid frame\n");
            continue; // Retry receiving
        }
        // Get the byte from the frame
        uint8_t byte = byte_from_bools(received_frame->frame.data);
        bool received_seq_num = calculate_seq_num(received_frame);
        printf("[receive_arq_frame] Received frame: (seq_num=%d, byte=%x)\n", received_seq_num, byte);

        // Check if the sequence number matches the expected one
        if (received_seq_num != expected_seq_num)
        {
            printf("[Transmit_arq_frame] Stale packet, seq_num mismatch (expected: %d, received: %d), retransmitting ACK\n", expected_seq_num, received_seq_num);
            arq_frame_t ack_frame = construct_arq_frame(byte, received_seq_num);
            send_raw_arq_frame(send_address, ack_frame);
            continue; // Retry receiving
        }
        
        // Send ACK for the received byte
        arq_frame_t ack_frame = construct_arq_frame(byte, received_seq_num);
        send_raw_arq_frame(receive_address, ack_frame); // Acknowledge the received byte

        printf("---------------------[receive_arq_frame] Valid frame received and ACK'd, moving on (seq_num=%d, byte=%x)------------------\n", received_seq_num, byte);

        return received_frame; // Return the valid frame
    }
}

// Ack protocol

/*

- Sender sends packet with seq_num 0, starts timer
- Receiver receives packet, checks validity, checks sequence num is expected (0)
    - if packet is invalid, drop it and keep listening
    - if packet is valid and seq num is expected, received a new packet so increment sequence number
    - if packet is valid but sequence num is less than expected, then it is retransmission so ACK it
- Sender waits for ACK
    - If ack with correct seq num is received before timeout, stop timer and proceed to send next packet ( increment sequence number)
    - If ack is not received before timeout, resend the packet with the same sequence number




*/