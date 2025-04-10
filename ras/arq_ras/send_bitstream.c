#include "print_utils.h"
#include "send.h"
#include <unistd.h>

int main(int argv, char *argc[])
{
    // Check if the user provided a target address
    if (argv < 2)
    {
        printf("Usage: %s <bitstream>\n", argc[0]);
        return 1;
    }
    // Convert input string of 1s and 0s to a bitstream
    const char *bitstream = argc[1];
    size_t bitstream_len = strlen(bitstream);
    // Allocate memory for the bitstream
    uint8_t *bitstream_bytes = malloc(bitstream_len);
    if (bitstream_bytes == NULL)
    {
        printf("Memory allocation failed\n");
        return 1;
    }
    // Convert the string to a bitstream
    for (size_t i = 0; i < bitstream_len; i++)
    {
        if (bitstream[i] == '1')
        {
            bitstream_bytes[i] = 1;
        }
        else if (bitstream[i] == '0')
        {
            bitstream_bytes[i] = 0;
        }
        else
        {
            printf("Invalid character in bitstream: %c\n", bitstream[i]);
            free(bitstream_bytes);
            return 1;
        }
    }

    // Send a stream of bits
    void *target_address = get_event_address(0, NULL);

    while (1)
    {
        // Send the bitstream
        for (size_t i = 0; i < bitstream_len; i++){
            send_bit(target_address, bitstream_bytes[i]);
            // Print the sent bit
            print_bit(bitstream_bytes[i], "Sent: ");
        }
        
    }

    return 0;
}