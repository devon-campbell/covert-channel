#include "utils.h"
#include "log.h"
#include "bit_channel.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <bit>\n", argv[0]);
        return 1;
    }

    int bit = atoi(argv[1]);
    if (bit != 0 && bit != 1) {
        printf("Bit must be 0 or 1\n");
        return 1;
    }

    // Initialize timing variables
    struct timespec start, end;
    double elapsed;
    long iterations = 0;
    const double test_duration = 5.0; // Run for 5 seconds

    printf("Starting benchmark for sending bit %d for %.1f seconds...\n", bit, test_duration);

    void *target_address = shared_file_address("/bin/ls", 0);

    // Start timing
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    do {
        send_bit(target_address, bit);
        iterations++;

        // Check if we've run for the desired duration
        clock_gettime(CLOCK_MONOTONIC, &end);
        elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    } while (elapsed < test_duration);

    // Calculate and print results
    double rate = iterations / elapsed;
    printf("Completed %ld send_bit calls in %.6f seconds\n", iterations, elapsed);
    printf("Average rate: %.2f bits per second\n", rate);

    return 0;
}