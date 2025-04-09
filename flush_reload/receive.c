
#include "receive.h"
#include "print_utils.h"

uint64_t tune_threshold(void *target_address)
{
    uint64_t total_hit_time = 0;
    uint64_t total_miss_time = 0;
    uint64_t total_hits = 0;
    uint64_t total_misses = 0;
    uint64_t iters = 1000;

    while (total_hits < iters && total_misses < iters)
    {
        // Randomly choose to flush or not
        bool do_flush = rand() % 2;
        if (do_flush)
        {
            // Flush the cache line to force a miss
            flush_event((uint64_t)target_address);
        }
        else
        {
            // Access the cache line
            // int ret = access_event();
            uint32_t ret = measure_one_block_access_time((uint64_t)target_address);
        }

        // Measure time to access the cache line
        uint32_t t = measure_one_block_access_time((uint64_t)target_address);

        if (do_flush)
        {
            total_misses++;
            total_miss_time += t;
        }
        else
        {
            total_hits++;
            total_hit_time += t;
        }
    }

    // Calculate average hit and miss times
    uint64_t avg_hit_time = total_hit_time / total_hits;
    uint64_t avg_miss_time = total_miss_time / total_misses;
    printf("Average hit time: %lu\n", avg_hit_time);
    printf("Average miss time: %lu\n", avg_miss_time);

    uint64_t miss_threashold = avg_hit_time + ((avg_miss_time -  avg_hit_time) / 4);
    printf("Threshold: %lu\n", miss_threashold);
    fflush(stdout);

    return miss_threashold;
}

inline bool receive_bit(void *target_address)
{
    uint64_t total_access_time = 0;
    uint64_t total_accesses = 0;
    uint32_t access_time;

    uint64_t threshold = get_threshold();
    // Wait until time step A
    uint32_t initial = start_sync();

    // Check until time step B
    while (!is_half_point())
    {
        access_time = measure_one_block_access_time((uint64_t)target_address);
        total_access_time += access_time;
        total_accesses++;
    }

    // Calculate average access time
    uint64_t avg_access_time = total_access_time / total_accesses;
    
    // print the average access time
//    printf("Avg access time: %lu\t\t", avg_access_time);
//    printf("Threshold: %lu\t\t", threshold);

    // Compare with threshold
    bool out_bit = (avg_access_time > threshold);

    // print the bit
    // printf("Received bit: %d\n", out_bit);
    // fflush(stdout);
  //  print_bit(out_bit, "Received: ");
    return out_bit;
}

// Receive a byte frame: state machine that waits for the preamble, start delimiter, reads data and parity
inline frame_t *receive_byte_frame(void *target_address, frame_t *frame_buf, uint64_t timeout)
{
    memset(frame_buf, 0, sizeof(frame_t));

    // In waiting state, look for the start delimiter with 8 bit shift register
    uint8_t shift_reg = 0;
    // start timeout time (microseconds)
    uint64_t start_time = rdtscp64();

    printf("[receive_byte_frame] Waiting until %lx cycles for start delimiter...\n", timeout);
    while (1) {
        // Shift in the next bit
        shift_reg = (shift_reg << 1) | receive_bit(target_address);
        
        // // Print shift register in binary
        // printf("Shift register: ");
        // for (int i = 7; i >= 0; i--) {
        //     printf("%d", (shift_reg >> i) & 1);
        // }
        // printf("\n");

        // fflush(stdout);
        // Check for start delimiter (0b10011010)
        if (shift_reg == START_DELIMITER) {
           // printf("New frame detected\n");
           // fflush(stdout);
            break; // Found start delimiter
        }
        // Check for timeout
       fflush(stdout);
        if (timeout > 0 && (rdtscp64() - start_time) > timeout) {
            printf("Timeout waiting for start delimiter\n");
            return NULL; // Timeout occurred
        }
    }

    // Save shift register as start delimiter
    byte_to_bools(shift_reg, frame_buf->start_delimiter);

    // In receiving state, read the next 8 bits for data and 1 bit for parity
    for (int i = 0; i < 8; i++) {
        frame_buf->data[i] = receive_bit(target_address);
    }
    frame_buf->parity = receive_bit(target_address);

    // Check frame validity
    if (!valid_frame(*frame_buf)) {
        printf("Invalid frame received\n");
        print_frame(*frame_buf);
        return NULL;
    }
   printf("[receive_byte_frame] Valid frame received\n");
    print_frame(*frame_buf);
    return frame_buf;
}