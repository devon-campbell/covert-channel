#include <stdio.h>
#include <unistd.h>
#include "utils.h"



int main() {
    while (1) {
        saturate_memory_bus(10000);
    }
    return 0;
}