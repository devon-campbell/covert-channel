#include "frame_channel.h"

// ARQ server waits for messages from client 

int main(int argv, char *argc[]){
    // Send a stream of bits
    void *send_address = get_client_address();
    void *receive_address = get_server_address();
    if (send_address == NULL || receive_address == NULL) {
        printf("Error: Could not get send or target address.\n");
        return 1;
    }
    // Open file for writing the bytestream
    FILE *recv_file = fopen("server_recv.txt", "w");
    if (recv_file == NULL) {
        perror("Failed to open sent.txt or recv.txt");
        return 1;
    }

    arq_frame_t frame_buf;
    bool seq_num = 0; // Sequence number for ARQ, toggles between 0 and 1
    while (1){
        arq_frame_t * ret = receive_arq_frame(receive_address, send_address, &frame_buf, seq_num);   
        seq_num = !seq_num; // Toggle sequence number for next frame
        if (ret == NULL) {
            printf("Failed to receive a valid frame.\n");
            continue; // Retry receiving
        }

        // Get the byte from the frame
        uint8_t byte = byte_from_bools(frame_buf.frame.data);
        printf("Received byte: %x\n", byte);
        
        // Write to file
        fputc(byte, recv_file);
        fflush(recv_file);  // Ensure data is written immediately
    }

    return 0;
}