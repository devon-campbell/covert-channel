#include "print_utils.h"
#include "send.h"


int main(int argv, char *argc[])
{
    // Check if the user provided a target address
    if (argv < 2)
    {
        printf("Usage: %s <bytestream>\n", argc[0]);
        return 1;
    } 
    const char *bytestream = argc[1];
    size_t bytestream_len = strlen(bytestream);
  
    // Send a stream of bits
    void *target_address = get_event_address(0, NULL);
    // Open file for writing the bytestream
    FILE *sent_file = fopen("sent.txt", "a");
    if (sent_file == NULL) {
        perror("Failed to open sent.txt");
        return 1;
    }
    // Write the bytestream to the file
    fprintf(sent_file, "%s", bytestream);
    fclose(sent_file); // Close the file after writing

    while (1)
    {
     
        for (size_t i = 0; i < bytestream_len; i++)
        {
            printf("\n\nSending byte: %c\n", bytestream[i]);
            send_byte_frame(target_address, (uint8_t) bytestream[i]);
            // Print the sent bit
            // print_stream(bytestream[i], "Sent: ");
        }
        
        // Sleep for 100 ms
        usleep(100000);
        exit(0);
    }

    return 0;
}