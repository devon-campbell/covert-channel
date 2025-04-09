#include "utils.h"



// Function to create a new Data structure
Data *make_data(int initial_size) {
    Data *data = (Data *)malloc(sizeof(Data));
    if (data == NULL) {
        perror("Failed to allocate memory for Data structure");
        return NULL;
    }
    data->data = (char *)malloc(initial_size);
    if (data->data == NULL) {
        perror("Failed to allocate memory for Data buffer");
        free(data);
        return NULL;
    }
    data->length = 0;
    data->size = initial_size;
    return data;
}

// Function to double the size of the Data buffer
Data *double_data(Data *data) {
    int new_size = data->size * 2;
    char *new_data = (char *)realloc(data->data, new_size);
    if (new_data == NULL) {
        perror("Failed to reallocate memory for Data buffer");
        return NULL;
    }
    data->data = new_data;
    data->size = new_size;
    return data;
}

// Function to free the Data structure
void free_data(Data *data) {
    if (data) {
        free(data->data);
        free(data);
    }
}

// New function to saturate memory bus with DRAM reads
void saturate_memory_bus(int duration_us) {
    // Create a large array that exceeds cache size
    // Using volatile to prevent compiler optimizations
    #define LARGE_ARRAY_SIZE (1024 * 1024 * 1024)  // 1GB - likely exceeds cache
    static volatile char* large_array = NULL;
    
    // Allocate on first use
    if (large_array == NULL) {
        large_array = (volatile char*)malloc(LARGE_ARRAY_SIZE);
        if (large_array == NULL) {
            perror("Failed to allocate memory for bus saturation");
            return;
        }
        // Initialize array
        for (int i = 0; i < LARGE_ARRAY_SIZE; i++) {
            large_array[i] = (char)i;
        }
    }
    
    // Read from random positions to avoid cache pattern prediction
    char dummy = 0;
    struct timespec start, current;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    do {
        for (int i = 0; i < 1000; i++) {
            // Random stride access to ensure DRAM reads
            int idx = (rand() % (LARGE_ARRAY_SIZE - 4096)) & ~0x3F;  // Align to 64 bytes
            dummy ^= large_array[idx];  // Force read and prevent optimization
        }
        clock_gettime(CLOCK_MONOTONIC, &current);
    } while ((current.tv_sec - start.tv_sec) * 1000000 + 
            (current.tv_nsec - start.tv_nsec) / 1000 < duration_us);
}

// Helper function to measure DRAM access time
uint64_t measure_dram_access_time() {
    // Create a larger array that won't fit entirely in cache
    #define TEST_ARRAY_SIZE (8 * 1024 * 1024)  // 8MB
    static volatile char* test_array = NULL;
    
    // Allocate on first use
    if (test_array == NULL) {
        test_array = (volatile char*)malloc(TEST_ARRAY_SIZE);
        if (test_array == NULL) {
            perror("Failed to allocate memory for timing test");
            return 0;
        }
        // Initialize array
        for (int i = 0; i < TEST_ARRAY_SIZE; i++) {
            test_array[i] = (char)i;
        }
    }
    
    uint64_t start, end;
    unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
    
    asm volatile ("CPUID\n\t"
                  "RDTSC\n\t"
                  "mov %%edx, %0\n\t"
                  "mov %%eax, %1\n\t"
                  : "=r" (cycles_high), "=r" (cycles_low)
                  :: "%rax", "%rbx", "%rcx", "%rdx");
    
    // Access memory in a pattern that forces DRAM access
    char dummy = 0;
    for (int i = 0; i < 100; i++) {
        int idx = (rand() % (TEST_ARRAY_SIZE - 4096)) & ~0x3F;
        dummy ^= test_array[idx];
    }
    
    asm volatile ("RDTSCP\n\t"
                  "mov %%edx, %0\n\t"
                  "mov %%eax, %1\n\t"
                  "CPUID\n\t"
                  : "=r" (cycles_high1), "=r" (cycles_low1)
                  :: "%rax", "%rbx", "%rcx", "%rdx");
    
    start = ((uint64_t)cycles_high << 32) | cycles_low;
    end = ((uint64_t)cycles_high1 << 32) | cycles_low1;
    
    return end - start;
}

// Modified function to send data through the DRAM contention channel
int send_data(const char *data, int n) {
    
    
    // Send data in chunks
    for (int i = 0; i < n; i++) {
        char byte = data[i];
        
        // Process each bit in the byte
        for (int bit = 0; bit < 8; bit++) {
            int bit_value = (byte >> bit) & 1;
            
            if (bit_value == 1) {
                // For '1' bit: saturate memory bus
                saturate_memory_bus(5000);  // Saturate for 5ms
            } else {
                // For '0' bit: do nothing, just wait
                usleep(5000);  // Wait equivalent time
            }
            
            
        }
        
    }
    
    
    return n;
}

// Modified function to receive data through the DRAM contention channel
Data * recv_data() {
    
    
    Data *data = make_data(1024);  // Initial buffer size
    
    int last_byte_marker = -1;
    
    while (1) {
        char byte = 0;
        
        // Process each bit in a byte
        for (int bit = 0; bit < 8; bit++) {
            
            // Measure DRAM access time
            uint64_t access_time = measure_dram_access_time();
            
            
            // IMPORTANT: Inverted logic - slow means '1', fast means '0'
            if (access_time > DRAM_THRESHOLD_NS) {
                // Slow access means memory bus contention, interpret as '1'
                byte |= (1 << bit);
            }
            // Fast access means no contention, interpret as '0'
            
            // Mark bit as received
        }
        
        // Check for end of byte marker
        
        data->data[data->length++] = byte;
        
        // Small delay before checking next byte
    }
    
    return data;
}