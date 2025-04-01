/* ***********************************************
 * benchmark.c
 * ----------------------------------------------
 * Performs benchmarking on RAS to determine what
 * its size is. Plot results with plot.py
 * *********************************************** */

#include <stdio.h>
#include <stdint.h>
#include <x86intrin.h>

#define MAX_DEPTH 40

static inline uint64_t rdtscp64() {
    unsigned aux;
    return __rdtscp(&aux);
}

static inline uint64_t timed_recurse(int depth, int count){
    if (count == depth) return rdtscp64();  // Timestamp at start of unraveling
    else {
        if (count == 0){  // Last to return (assuming depth > 0)
            uint64_t start = timed_recurse(depth, ++count);
            return rdtscp64() - start;
        }
        else return timed_recurse(depth, ++count);
    }
}

void print_results(const char *filename, uint64_t values[], size_t size) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Failed to open file");
        return;
    }

    for (size_t i = 0; i < size; i++) {
        fprintf(file, "%llu\n", (unsigned long long)values[i]);
    }

    fclose(file);
}

int main(){
    uint64_t times[MAX_DEPTH];

    for (int depth = 1; depth <= MAX_DEPTH; depth++){
        times[depth] = timed_recurse(depth, 0);
    }

    print_results("results.txt", times, MAX_DEPTH);
    return 0;
}