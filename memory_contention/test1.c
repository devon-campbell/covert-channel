#include "utils.h"

#include <stdio.h>

int main(){
    while (1) {
        unsigned long access_time = measure_dram_access_time();
        printf("DRAM Access Time: %lu cycles\n", access_time);
        usleep(333333); // Sleep for approximately 1/3 second
    }
}