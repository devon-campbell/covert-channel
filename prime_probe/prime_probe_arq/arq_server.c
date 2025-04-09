#include "arq_channel.h"
#include "phy.h"  
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * ARQ server listens for messages from the client using Prime+Probe.
 */
int main(void) {
    phy_init();

    FILE *recv_file = fopen("server_recv.txt", "w");
    if (!recv_file) {
        perror("server_recv.txt");
        return 1;
    }

    arq_frame_t frame_buf;
    bool seq_num = 0;

    while (1) {
        arq_frame_t *ret = receive_arq_frame(&frame_buf, seq_num);
        if (!ret) continue;

        seq_num ^= 1;  

        uint8_t byte = byte_from_bools(frame_buf.frame.data);
        // printf("[server] Received byte: 0x%02x ('%c')\n", byte, byte);

        fputc(byte, recv_file);
        fflush(recv_file);
    }

    fclose(recv_file);
    return 0;
}