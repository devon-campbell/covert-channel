/* **********************************************
 * sender.c
 * ----------------------------------------------
 * Sends bits to receiver by flushing the RAS.
 * Calls heavily nested function to flood RAS with
 * wrong return addresses.
 * ********************************************** */

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>

static inline void flush_ras(int count, int threshold){
    if (count == threshold) {
        sched_yield();
        return;
    }
    else flush_ras(++count, threshold);
}

int main(){
    printf("Continuously flushing RAS repeatedly with 16 nested calls...\n");
    while (1){
        flush_ras(0, 16);
    }
}