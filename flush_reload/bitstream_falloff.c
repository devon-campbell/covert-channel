#include "print_utils.h"

#include "receive.h"
#include "send.h"

#define FORCE_INLINE __attribute__((always_inline)) inline

// Benchmark the bit accuracy falloff without frames

#define PATTERN 0b10110110

// Receive stream of bits and print them
int receive_sample(int sample_count, bool *output_buffer)
{

    void *target_address = get_event_address(0, NULL);
    uint8_t shift_reg = 0;

    while (1)
    {
        // Shift in the next bit
        bool bit = receive_bit(target_address);
        shift_reg = (shift_reg << 1) | bit;
        // print_bit(bit, "Received: ");

        // Once pattern detected, break
        if (shift_reg == PATTERN)
        {
            fprintf(stderr, "Pattern detected: %x\n", shift_reg);
            break; // Found pattern
        }
    }

    // Save initial shift reg in out buffer
    byte_to_bools(shift_reg, output_buffer);
    output_buffer += 8;
    sample_count -= 8;

    while (sample_count > 0)
    {
        // Recieve next bit
        *output_buffer = receive_bit(target_address);
        // print_bit(*output_buffer, "Received: ");
        output_buffer++;
        sample_count--;
    }

    return 0;
}

int send_sample(bool *output_buffer, int sample_count)
{

    // Send a stream of bits
    void *target_address = get_event_address(0, NULL);

    // Send the bitstream
    for (int i = 0; i < sample_count; i++)
    {
        send_bit(target_address, output_buffer[i]);
        // Print the sent bit
        print_bit(output_buffer[i], "Sent: ");
    }

    return 0;
}

// {

//     size_t num_bits = num_pattern_reps * 8;
//     // Allocate memory for the bitstream
//     bool *bitstream = malloc(num_bits * sizeof(bool));
//     if (bitstream == NULL)
//     {
//         printf("Memory allocation failed\n");
//         return 1;
//     }
//     // Convert the string to a bitstream
//     for (size_t i = 0; i < num_pattern_reps; i++)
//     {
//         byte_to_bools(PATTERN, bitstream + (i * sizeof(bool) * 8));
//     }

//     // Send a stream of bits
//     void *target_address = get_event_address(0, NULL);

//     // Send the bitstream
//     for (size_t i = 0; i < num_bits; i++)
//     {
//         send_bit(target_address, bitstream[i]);
//         // Print the sent bit
//         print_bit(bitstream[i], "Sent: ");
//     }

//     return 0;
// }

#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SHM_NAME "/bitstream_shm"

int main(int argc, char *argv[])
{

    // Retrieve argv[1] as the number of pattern repetitions
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <num_pattern_reps>\n", argv[0]);
        return 1;
    }
    int num_pattern_reps = atoi(argv[1]);
    if (num_pattern_reps <= 0)
    {
        fprintf(stderr, "Invalid number of pattern repetitions: %d\n", num_pattern_reps);
        return 1;
    }
    printf("Number of pattern repetitions: %d\n", num_pattern_reps);

    int sample_count = num_pattern_reps * 8;

    // Create shared memory for the receiver to store results
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open failed");
        return 1;
    }

    // Set size for the shared memory segment
    if (ftruncate(shm_fd, sample_count * sizeof(bool)) == -1)
    {
        perror("ftruncate failed");
        shm_unlink(SHM_NAME);
        return 1;
    }

    // Map the shared memory segment
    bool *shared_buffer = mmap(NULL, sample_count * sizeof(bool),
                               PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_buffer == MAP_FAILED)
    {
        perror("mmap failed");
        shm_unlink(SHM_NAME);
        return 1;
    }

    // Fork to create receiver process
    pid_t receiver_pid = fork();

    if (receiver_pid == -1)
    {
        perror("Fork failed");
        munmap(shared_buffer, sample_count * sizeof(bool));
        shm_unlink(SHM_NAME);
        return 1;
    }
    else if (receiver_pid == 0)
    {
        // Child process (receiver)
        printf("Receiver started, waiting for pattern...\n");
        receive_sample(sample_count, shared_buffer);
        exit(0);
    }

    // Parent process continues

    // Sleep to let receiver initialize
    sleep(1);

    // Create expected bit stream for comparison
    bool *expected_bits = malloc(sample_count * sizeof(bool));

    int num_prefix_patterns = 2;
    int index = 0;
    for (size_t i = 0; i < num_prefix_patterns; i++)
    {
        byte_to_bools(PATTERN, expected_bits + index);
        index += 8;
    }

    for (size_t i = index; i < (size_t)sample_count; i++)
    {
        expected_bits[i] = (rand() % 2) == 1;
    }

    send_sample(expected_bits, sample_count);
    // Wait for receiver to finish
    int status;
    waitpid(receiver_pid, &status, 0);

    // Compare expected with received bits and save errors to CSV
    int errors = 0;

    // Check if file exists to determine if we need to write headers
    bool file_exists = (access("bit_errors.csv", F_OK) == 0);

    FILE *csv_file = fopen("bit_errors.csv", "a");
    if (!csv_file)
    {
        perror("Error opening CSV file");
    }
    else
    {
        // Write CSV header only if creating a new file
        if (!file_exists)
        {
            fprintf(csv_file, "bit_index,error\n");
        }

        // Compare bits and log errors to CSV
        for (int i = 0; i < sample_count; i++)
        {
            if (shared_buffer[i] != expected_bits[i])
            {
                errors++;
                printf("Bit %d: expected %d, got %d\n", i, expected_bits[i], shared_buffer[i]);
                fprintf(csv_file, "%d,1\n", i);
            }
            else
            {
                fprintf(csv_file, "%d,0\n", i);
            }
        }
        fclose(csv_file);
    }

    printf("Comparison complete: %d errors out of %d bits (%.2f%%)\n",
           errors, sample_count, (errors * 100.0) / sample_count);

    // Clean up
    munmap(shared_buffer, sample_count * sizeof(bool));
    shm_unlink(SHM_NAME);

    return 0;
}