#ifndef ARQ_FRAMES_H
#define ARQ_FRAMES_H

#include "utils.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>

/* FRAME / FRAME STRUCTURE */
#define START_DELIMITER (0b10011010)
#define RESET_DELIMITER (0b10101101)

// Underlying frame structure
typedef struct frame
{
    bool start_delimiter[8]; // Four bit start delimiter of 1001101
    bool data[8];            // Data bits
    bool parity;
} frame_t;
#define FRAME_BITLEN (17)

// Frame with additonal ARQ metadata
typedef struct
{
    bool seq_num_1[6];
    frame_t frame;
    bool seq_num_2[4];
    bool dup_data[8];
    bool seq_num_3[4];
    bool dup_data2[8];
    bool seq_num_4[4];
    bool dup_data3[8];
    bool reset_gap[8];       // Always 1's, receiver will allow this to be of length 0 to 16
    bool reset_delimiter[8]; // Reset delimiter (0b10101101)
    bool dup_data4[8];
    bool seq_num_5[4];
    bool dup_data5[8];

    // bool crc[8]; // CRC bits
} arq_frame_t;
#define ARQ_FRAME_BITLEN (6 + FRAME_BITLEN + 4 + 8 + 4 + 8 + 4 + 8 + 8 + 8 + 8 + 4 + 8)
// (sizeof(seq_num_1) + FRAME_BITLEN + sizeof(seq_num_2) + sizeof(dup_data) + sizeof(seq_num_3) + sizeof(dup_data2) + sizeof(seq_num_4))

#define NUM_SEQ_DUPS ((3 + 2 + 2 + 2 + 2 + 2))
// ((sizeof(seq_num_1) + sizeof(seq_num_2) + sizeof(seq_num_3) + sizeof(seq_num_4)))
static inline frame_t construct_frame(uint8_t data)
{
    frame_t f;
    memset(&f, 0, sizeof(frame_t));

    // Start delimiter (8 bits, 1101101)
    uint8_t start_delimiter = START_DELIMITER;
    byte_to_bools(start_delimiter, f.start_delimiter);

    byte_to_bools(data, f.data);

    // Calculate parity (even parity)
    bool parity = 0;
    for (int i = 0; i < 8; i++)
    {
        parity ^= ((data >> i) & 1);
    }

    f.parity = parity;
    return f;
}

static inline arq_frame_t construct_arq_frame(uint8_t data, uint8_t seq_num)
{
    arq_frame_t f;
    memset(&f, 0, sizeof(arq_frame_t));

    for (unsigned long i = 0; i < sizeof(f.seq_num_1); i++)
    {
        f.seq_num_1[i] = (bool)seq_num;
    }
    // byte_to_two_bit(seq_num, f.seq_num_1);
    // byte_to_two_bit(seq_num, f.seq_num_1 + 2);
    // byte_to_two_bit(seq_num, f.seq_num_1 + 4);

    f.frame = construct_frame(data);

    // byte_to_two_bit(seq_num, f.seq_num_2);
    // byte_to_two_bit(seq_num, f.seq_num_2 + 2);
    for (unsigned long i = 0; i < sizeof(f.seq_num_2); i++)
    {
        f.seq_num_2[i] = seq_num;
    }

    byte_to_bools(data, f.dup_data);

    // byte_to_two_bit(seq_num, f.seq_num_3);
    // byte_to_two_bit(seq_num, f.seq_num_3 + 2);
    for (unsigned long i = 0; i < sizeof(f.seq_num_3); i++)
    {
        f.seq_num_3[i] = (bool)seq_num;
    }

    byte_to_bools(data, f.dup_data2);

    // byte_to_two_bit(seq_num, f.seq_num_4);
    // byte_to_two_bit(seq_num, f.seq_num_4 + 2);

    for (unsigned long i = 0; i < sizeof(f.seq_num_4); i++)
    {
        f.seq_num_4[i] = (bool)seq_num;
    }
    byte_to_bools(data, f.dup_data3);

    // Reset gap (always 1's)
    for (unsigned long i = 0; i < sizeof(f.reset_gap); i++)
    {
        f.reset_gap[i] = 1;
    }
    // Reset delimiter (0b10101101)
    uint8_t reset_delimiter = RESET_DELIMITER;
    byte_to_bools(reset_delimiter, f.reset_delimiter);

    byte_to_bools(data, f.dup_data4);

    // byte_to_two_bit(seq_num, f.seq_num_5);
    // byte_to_two_bit(seq_num, f.seq_num_5 + 2);
    for (unsigned long i = 0; i < sizeof(f.seq_num_5); i++)
    {
        f.seq_num_5[i] = (bool)seq_num;
    }

    byte_to_bools(data, f.dup_data5);

    return f;
}

static bool *flatten_arq_frame(arq_frame_t *frame, bool *buffer)
{
    // Convert frame to array of bools
    bool *curr_buffer = buffer;
    memcpy(curr_buffer, frame->seq_num_1, sizeof(frame->seq_num_1));
    curr_buffer += sizeof(frame->seq_num_1) / sizeof(bool);
    memcpy(curr_buffer, frame->frame.start_delimiter, sizeof(frame->frame.start_delimiter));
    curr_buffer += sizeof(frame->frame.start_delimiter) / sizeof(bool);
    memcpy(curr_buffer, frame->frame.data, sizeof(frame->frame.data));
    curr_buffer += sizeof(frame->frame.data) / sizeof(bool);
    memcpy(curr_buffer, &frame->frame.parity, sizeof(frame->frame.parity));
    curr_buffer += sizeof(frame->frame.parity) / sizeof(bool);
    memcpy(curr_buffer, frame->seq_num_2, sizeof(frame->seq_num_2));
    curr_buffer += sizeof(frame->seq_num_2) / sizeof(bool);
    memcpy(curr_buffer, frame->dup_data, sizeof(frame->dup_data));
    curr_buffer += sizeof(frame->dup_data) / sizeof(bool);
    memcpy(curr_buffer, frame->seq_num_3, sizeof(frame->seq_num_3));
    curr_buffer += sizeof(frame->seq_num_3) / sizeof(bool);
    memcpy(curr_buffer, frame->dup_data2, sizeof(frame->dup_data2));
    curr_buffer += sizeof(frame->dup_data2) / sizeof(bool);
    memcpy(curr_buffer, frame->seq_num_4, sizeof(frame->seq_num_4));
    curr_buffer += sizeof(frame->seq_num_4) / sizeof(bool);
    memcpy(curr_buffer, frame->dup_data3, sizeof(frame->dup_data3));
    curr_buffer += sizeof(frame->dup_data3) / sizeof(bool);
    memcpy(curr_buffer, frame->reset_gap, sizeof(frame->reset_gap));
    curr_buffer += sizeof(frame->reset_gap) / sizeof(bool);
    memcpy(curr_buffer, frame->reset_delimiter, sizeof(frame->reset_delimiter));
    curr_buffer += sizeof(frame->reset_delimiter) / sizeof(bool);
    memcpy(curr_buffer, frame->dup_data4, sizeof(frame->dup_data4));
    curr_buffer += sizeof(frame->dup_data4) / sizeof(bool);
    memcpy(curr_buffer, frame->seq_num_5, sizeof(frame->seq_num_5));
    curr_buffer += sizeof(frame->seq_num_5) / sizeof(bool);
    memcpy(curr_buffer, frame->dup_data5, sizeof(frame->dup_data5));
    curr_buffer += sizeof(frame->dup_data5) / sizeof(bool);
    return buffer;
}
static inline bool *frame_data_bools(arq_frame_t *frame, bool *buffer)
{
    bool invalidate = false;
    // Extract the data from the frame, and vote on each bit
    for (int i = 0; i < 8; i++)
    {
        int votes_one = 0;

        if (frame->frame.data[i])
        {
            votes_one++;
        }
        if (frame->dup_data[i])
        {
            votes_one++;
        }
        if (frame->dup_data2[i])
        {
            votes_one++;
        }
        if (frame->dup_data3[i])
        {
            votes_one++;
        }
        if (frame->dup_data4[i])
        {
            votes_one++;
        }
        if (frame->dup_data5[i])
        {
            votes_one++;
        }

        // If 5/6 agree on a bit, set it to that value
        if (votes_one > 4) // 5/6 or 6/6
        {
            buffer[i] = 1;
        }
        else if (votes_one < 2) // 1/6 or 0/6
        {
            buffer[i] = 0;
        }
        else
        {
            // If 2/4, invalidate
            invalidate = true;
            buffer[i] = 0;
        }

        buffer[i] = (votes_one > 1); // Majority vote
    }

    // memcpy(buffer, final_bits, sizeof(final_bits));
    if (invalidate)
    {
        return NULL;
    }
    return buffer;
}

static uint8_t frame_data(arq_frame_t *frame)
{
    bool final_bits[8] = {0};
    frame_data_bools(frame, final_bits);
    return byte_from_bools(final_bits);
    // Extract the data from the frame, and vote on each bit
    // bool final_bits[8] = {0};
    // for (int i = 0; i < 8; i++)
    // {
    //     int votes_one = 0;
    extern FILE *log_fd;

    //     if (frame->frame.data[i])
    //     {
    //         votes_one++;
    //     }
    //     if (frame->dup_data[i])
    //     {
    //         votes_one++;
    //     }
    //     if (frame->dup_data2[i])
    //     {
    //         votes_one++;
    //     }

    //     final_bits[i] = (votes_one > 2); // Majority vote
    // }

    // return byte_from_bools(final_bits);
}

static int frame_seq_one_votes(arq_frame_t *f)
{
    // Retunr the number of ones in all seq bits
    int votes_one = 0;
    for (unsigned long i = 0; i < sizeof(f->seq_num_1); i++)
    {
        if (f->seq_num_1[i])
        {
            votes_one++;
        }
    }
    for (unsigned long i = 0; i < sizeof(f->seq_num_2); i++)
    {
        if (f->seq_num_2[i])
        {
            votes_one++;
        }
    }
    for (unsigned long i = 0; i < sizeof(f->seq_num_3); i++)
    {
        if (f->seq_num_3[i])
        {
            votes_one++;
        }
    }
    for (unsigned long i = 0; i < sizeof(f->seq_num_4); i++)
    {
        if (f->seq_num_4[i])
        {
            votes_one++;
        }
    }
    for (unsigned long i = 0; i < sizeof(f->seq_num_5); i++)
    {
        if (f->seq_num_5[i])
        {
            votes_one++;
        }
    }
    return votes_one;
}
// return 1 if past high threshold, 0 if past low threshold, else -1
static int seq_number_voting(arq_frame_t *f)
{

    int num_one_votes = frame_seq_one_votes(f);
    int num_votes = sizeof(f->seq_num_1) + sizeof(f->seq_num_2) + sizeof(f->seq_num_3) + sizeof(f->seq_num_4) + sizeof(f->seq_num_5);

    // Threshold of 2/3s
    int one_threshold_high = (num_votes * 2) / 3;
    int one_threshold_low = (num_votes * 1) / 3;

    // If more than threshold of votes are 1s, return 1, else return 0
    if (num_one_votes > one_threshold_high)
    {
        return 1;
    }
    else if (num_one_votes < one_threshold_low)
    {
        return 0;
    }
    else
    {
        return -1; // Invalid
    }
}

static uint8_t frame_seq_num_onebit(arq_frame_t *f)
{

    return seq_number_voting(f) == 1 ? 1 : 0;
    // int num_one_votes = frame_seq_one_votes(f);
    // int num_votes = sizeof(f->seq_num_1) + sizeof(f->seq_num_2) + sizeof(f->seq_num_3) + sizeof(f->seq_num_4) + sizeof(f->seq_num_5);

    // // Threshold of 2/3s
    // int one_threshold_high = (num_votes * 2) / 3;

    // // If more than threshold of votes are 1s, return 1, else return 0
    // return (num_one_votes > one_threshold_high) ? 1 : 0;
}

static uint8_t frame_seq_num_twobit(arq_frame_t *f)
{
    uint8_t rep_values[NUM_SEQ_DUPS] = {0};
    int idx = 0;

    rep_values[idx++] = two_bit_to_byte(f->seq_num_1);
    rep_values[idx++] = two_bit_to_byte(f->seq_num_1 + 2);
    rep_values[idx++] = two_bit_to_byte(f->seq_num_1 + 4);

    rep_values[idx++] = two_bit_to_byte(f->seq_num_2);
    rep_values[idx++] = two_bit_to_byte(f->seq_num_2 + 2);

    rep_values[idx++] = two_bit_to_byte(f->seq_num_3);
    rep_values[idx++] = two_bit_to_byte(f->seq_num_3 + 2);

    rep_values[idx++] = two_bit_to_byte(f->seq_num_4);
    rep_values[idx++] = two_bit_to_byte(f->seq_num_4 + 2);

    rep_values[idx++] = two_bit_to_byte(f->seq_num_5);
    rep_values[idx++] = two_bit_to_byte(f->seq_num_5 + 2);

    // Take majority vote to determine overall bit
    uint8_t votes[4] = {0};

    debug_np("rep_values: ");
    for (uint8_t i = 0; i < NUM_SEQ_DUPS; i++)
    {
        debug_np("%d ", rep_values[i]);
        votes[rep_values[i]]++;
    }
    debug_np("\n");

    // Get max vote
    uint8_t max_vote_index = 0;
    int max_vote_value = 0;
    for (uint8_t i = 0; i < 4; i++)
    {
        if (votes[i] > max_vote_value)
        {
            max_vote_value = votes[i];
            max_vote_index = i;
        }
    }

    return max_vote_index;
}

static inline uint8_t frame_seq_num(arq_frame_t *f)
{
    // Get the sequence number from the frame
    // return byte_from_bools(f->seq_num_1);
    return frame_seq_num_onebit(f);
    // return frame_seq_num_twobit(f);
}

//     // Take majority vote to determine overall bit
//     int votes_zero = 0;
//     int votes_one = 0;
//     for (unsigned long i = 0; i < sizeof(f->seq_num_1); i++)
//     {
//         if (f->seq_num_1[i])
//         {
//             votes_one++;
//         }
//         else
//         {
//             votes_zero++;
//         }
//     }
//     for (unsigned long i = 0; i < sizeof(f->seq_num_2); i++)
//     {
//         if (f->seq_num_2[i])
//         {
//             votes_one++;
//         }
//         else
//         {
//             votes_zero++;
//         }
//     }
//     for (unsigned long i = 0; i < sizeof(f->seq_num_3); i++)
//     {
//         if (f->seq_num_3[i])
//         {
//             votes_one++;
//         }
//         else
//         {
//             votes_zero++;
//         }
//     }
//     for (unsigned long i = 0; i < sizeof(f->seq_num_4); i++)
//     {
//         if (f->seq_num_4[i])
//         {
//             votes_one++;
//         }
//         else
//         {
//             votes_zero++;
//         }
//     }

//     // Majority vote: if more than half are 1s, return 1, else return 0
//     return (votes_one > votes_zero);
// }

static FORCE_INLINE bool valid_frame(arq_frame_t *f)
{
    // Check if the start delimiter is valid
    bool voted_bits[8] = {0};
    void *data_ptr = frame_data_bools(f, voted_bits);
    if (data_ptr == NULL)
    {
        return false;
    }
    // Check seq num voting
    int seq_num_vote = seq_number_voting(f);
    if (seq_num_vote == -1)
    {
        return false;
    }

    // Check parity
    bool parity = calculate_parity(voted_bits, 8);
    return parity == f->frame.parity;
}
#ifndef QUIET
static FORCE_INLINE void print_frame(frame_t f)
{
    debug_np("Frame: | ");
    debug_np("  ");
    debug_np("Start Delim: 0x%02X", byte_from_bools(f.start_delimiter));
    debug_np(" |  ");
    debug_np("Data: 0x%02X (%c)", byte_from_bools(f.data), byte_from_bools(f.data));
    debug_np(" |  ");
    debug_np("Parity: %d (true parity=%d)", f.parity, calculate_parity(f.data, 8));
    debug_np("\n");
    debug_np("Bits: ");

    print_bools(f.start_delimiter, sizeof(f.start_delimiter));
    print_bools(f.data, sizeof(f.data));
    debug_np("%d", f.parity);

    debug_np("\n\n");
    fflush(stdout);
}

static FORCE_INLINE void print_arq_flat(arq_frame_t f)
{
    debug_np("Flattened Bits: ");
    bool flattened_arq_frame[ARQ_FRAME_BITLEN] = {0};
    bool *curr_buffer = flatten_arq_frame(&f, flattened_arq_frame);

    // Seq num 1
    debug_np("%d%d ", curr_buffer[0], curr_buffer[1]);
    debug_np("%d%d ", curr_buffer[2], curr_buffer[3]);
    debug_np("%d%d | ", curr_buffer[4], curr_buffer[5]);

    // Start Delimiter
    debug_np("%d%d%d%d%d%d%d%d", curr_buffer[6], curr_buffer[7], curr_buffer[8], curr_buffer[9], curr_buffer[10], curr_buffer[11], curr_buffer[12], curr_buffer[13]);
    debug_np(" | ");
    // Data
    debug_np("%d%d%d%d%d%d%d%d", curr_buffer[14], curr_buffer[15], curr_buffer[16], curr_buffer[17], curr_buffer[18], curr_buffer[19], curr_buffer[20], curr_buffer[21]);
    debug_np(" | ");
    // Parity
    debug_np("%d", curr_buffer[22]);
    debug_np(" | ");

    // Seq num 2
    debug_np("%d%d ", curr_buffer[23], curr_buffer[24]);
    debug_np("%d%d | ", curr_buffer[25], curr_buffer[26]);
    // Dup Data
    debug_np("%d%d%d%d%d%d%d%d", curr_buffer[27], curr_buffer[28], curr_buffer[29], curr_buffer[30], curr_buffer[31], curr_buffer[32], curr_buffer[33], curr_buffer[34]);
    debug_np(" | ");
    // Seq num 3
    debug_np("%d%d ", curr_buffer[35], curr_buffer[36]);
    debug_np("%d%d | ", curr_buffer[37], curr_buffer[38]);
    // Dup Data 2
    debug_np("%d%d%d%d%d%d%d%d", curr_buffer[39], curr_buffer[40], curr_buffer[41], curr_buffer[42], curr_buffer[43], curr_buffer[44], curr_buffer[45], curr_buffer[46]);
    debug_np(" | ");
    // Seq num 4
    debug_np("%d%d ", curr_buffer[47], curr_buffer[48]);
    debug_np("%d%d ", curr_buffer[49], curr_buffer[50]);
    debug_np("| ");
    // Dup Data 3
    debug_np("%d%d%d%d%d%d%d%d", curr_buffer[51], curr_buffer[52], curr_buffer[53], curr_buffer[54], curr_buffer[55], curr_buffer[56], curr_buffer[57], curr_buffer[58]);
    debug_np(" | ");
    // Reset Gap
    debug_np("%d%d%d%d%d%d%d%d", curr_buffer[59], curr_buffer[60], curr_buffer[61], curr_buffer[62], curr_buffer[63], curr_buffer[64], curr_buffer[65], curr_buffer[66]);
    debug_np(" | ");
    // Reset Delimiter
    debug_np("%d%d%d%d%d%d%d%d", curr_buffer[67], curr_buffer[68], curr_buffer[69], curr_buffer[70], curr_buffer[71], curr_buffer[72], curr_buffer[73], curr_buffer[74]);
    debug_np(" | ");
    // Dup Data 4
    debug_np("%d%d%d%d%d%d%d%d", curr_buffer[75], curr_buffer[76], curr_buffer[77], curr_buffer[78], curr_buffer[79], curr_buffer[80], curr_buffer[81], curr_buffer[82]);
    debug_np(" | ");
    // Seq num 5
    debug_np("%d%d ", curr_buffer[83], curr_buffer[84]);
    debug_np("%d%d | ", curr_buffer[85], curr_buffer[86]);
    // Dup Data 5
    debug_np("%d%d%d%d%d%d%d%d", curr_buffer[87], curr_buffer[88], curr_buffer[89], curr_buffer[90], curr_buffer[91], curr_buffer[92], curr_buffer[93], curr_buffer[94]);

    debug_np("\n");
}

static inline void print_arq_frame(arq_frame_t f)
{
    debug_np("Dumping contents of ARQ Frame (Seq No %d, Byte %02X)\n", frame_seq_num(&f), frame_data(&f));
    print_frame(f.frame);
    debug_np("\nDup Data: 0x%02X (%c)\n", byte_from_bools(f.dup_data), byte_from_bools(f.dup_data));
    debug_np("Dup Data2: 0x%02X (%c)\n", byte_from_bools(f.dup_data2), byte_from_bools(f.dup_data2));
    debug_np("Dup Data3: 0x%02X (%c)\n", byte_from_bools(f.dup_data3), byte_from_bools(f.dup_data3));
    debug_np("\nExtra SEQ Bits: ");
    print_bools(f.seq_num_1, sizeof(f.seq_num_1));
    print_bools(f.seq_num_2, sizeof(f.seq_num_2));
    print_bools(f.seq_num_3, sizeof(f.seq_num_3));
    print_bools(f.seq_num_4, sizeof(f.seq_num_4));
    print_bools(f.seq_num_5, sizeof(f.seq_num_5));
    debug_np("\nDup Data: ");
    print_bools(f.dup_data, sizeof(f.dup_data));
    debug_np("\nDup Data2: ");
    print_bools(f.dup_data2, sizeof(f.dup_data2));
    debug_np("\nDup Data3: ");
    print_bools(f.dup_data3, sizeof(f.dup_data3));
    debug_np("\nDup Data4: ");
    print_bools(f.dup_data4, sizeof(f.dup_data4));

    debug_np("\n\n");
    print_arq_flat(f);
    // debug_np("Bits: ");
    // bool flattened_arq_frame[ARQ_FRAME_BITLEN] = {0};
    // bool *curr_buffer = flatten_arq_frame(&f, flattened_arq_frame);
    // print_bools(flattened_arq_frame, sizeof(flattened_arq_frame));

    debug_np("\n------------------- End of ARQ Frame ------------------------\n");

}

#endif // QUIET
#endif // ARQ_FRAMES_H