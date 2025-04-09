#include "print_utils.h"

#include "receive.h"

// A CD  GH  KLMNOPABCD FGHIJKLMO P
// ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP


// Receive stream of bits and print them
int main(int argc, char *argv[]) {
    void *target_address = get_event_address(0, NULL);
    frame_t frame_buf;
    
    // Determine output filename
    const char *filename = "recv.txt";
    if (argc > 1) {
        filename = argv[1];
    }
    
    // Open file for writing
    FILE *outfile = fopen(filename, "w");
    if (outfile == NULL) {
        perror("Error opening output file");
        return 1;
    }
    
    while (1) {
        frame_t *frame = receive_byte_frame(target_address, &frame_buf, 0);
        if (frame == NULL) {
            printf("Invalid frame received\n");
            continue;
        }
        
        // Get byte from frame
        uint8_t byte = byte_from_bools(frame->data);
        
        // Print to console
        print_stream(byte, "Received byte: ");
        
        // Write to file
        fputc(byte, outfile);
        fflush(outfile);  // Ensure data is written immediately
    }
    
    // Note: This will never be reached unless we add a break condition
    fclose(outfile);
    return 0;
}
