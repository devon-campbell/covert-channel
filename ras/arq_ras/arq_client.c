#include "covert_utils.h"
#include "send.h"
#include "receive.h"
#include "frame_channel.h"

// ARQ client sends messages to server 

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
    void *send_address = get_client_address();
    void *receive_address = get_server_address();
    // get_threshold(); // Ensure the threshold is tuned for the receive address
    if (send_address == NULL || receive_address == NULL) {
        printf("Error: Could not get send or target address.\n");
        return 1;
    }
    // Open file for writing the bytestream
    FILE *sent_file = fopen("client_sent.txt", "w");
    FILE *recv_file = fopen("client_recv.txt", "w");
    if (sent_file == NULL || recv_file == NULL) {
        perror("Failed to open sent.txt or recv.txt");
        return 1;
    }
    // Write the bytestream to the file
    fprintf(sent_file, "%s", bytestream);
    fclose(sent_file); // Close the file after writing

    bool seq_num = 0; // Sequence number for ARQ, toggles between 0 and 1
    while (1)
    {
     
        for (size_t i = 0; i < bytestream_len; i++)
        {
            printf("\n\nSending byte: %c\n", bytestream[i]);
            transmit_arq_frame(send_address, receive_address, (uint8_t) bytestream[i], seq_num);
            seq_num = !seq_num; // Toggle sequence number for next frame
            // Print the sent bit
            // print_stream(bytestream[i], "Sent: ");
        }
        
        // Sleep for 100 ms
        usleep(100000);
        exit(0);
    }

    return 0;
}