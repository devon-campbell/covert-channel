/* ***********************************************
 * receiver.c
 * ----------------------------------------------
 * RAS covert channel receiver - calls nested  
 * function, yields control, and times return
 * to detect mispredictions caused by sender's RAS
 * flush.
 * *********************************************** */

#include <sched.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <x86intrin.h>

// Duration of sleep in cycles
#define SLEEP_DUR 10000
// Duration of test recursion
#define REC_DEPTH 16
// Will depend on depth of recursion, size of RAS, CPU, etc.
// Use threshold.c to fine tune
#define THRESHOLD 650

// Read timestamp counter
static inline uint64_t rdtscp64() {
    unsigned aux;
    return __rdtscp(&aux);
}

// Fill RAS and measure return time after an interval
static inline uint64_t recurse_and_wait(int depth, int count, uint64_t wait_cycles){
    if (count == depth){
        // Yield to potential transmitter process
        sched_yield();
        return rdtscp64();
    }else{
        if (count == 0){  // Last to return (assuming depth > 0)
            uint64_t start = recurse_and_wait(depth, count+1, wait_cycles);
            return rdtscp64() - start;
        }
        else return recurse_and_wait(depth, count+1, wait_cycles);
    }
}

int main(){
    while(1){
        uint64_t delay = recurse_and_wait(REC_DEPTH, 0, SLEEP_DUR);
        int bit = (delay > THRESHOLD) ? 1 : 0;
        printf("%d", bit);
        // printf(" | Delay: %lu\n", delay);
        fflush(stdout);
    }

    return 0;
}