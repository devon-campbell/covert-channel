#include "print_utils.h"
#include "receive.h"


// Receive stream of bits and print them
int main() {

    void *target_address = get_event_address(0, NULL);
    while (1) {
        bool bit = receive_bit(target_address);
        print_bit(bit, "Received: ");
    }

}
