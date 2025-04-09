#include "arq_channel.h"
#include "phy.h"  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * ARQ client sends messages to server using Prime+Probe.
 */
int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: %s <bytestream>\n", argv[0]);
        return 1;
    }

    const char *bytestream = argv[1];
    size_t len = strlen(bytestream);

    // Setup: Prime+Probe + threshold
    phy_init();

    // Save sent message to file
    FILE *sent_file = fopen("client_sent.txt", "w");
    if (!sent_file) {
        perror("client_sent.txt");
        return 1;
    }
    fprintf(sent_file, "%s", bytestream);
    fclose(sent_file);

    bool seq_num = 0;

    for (size_t i = 0; i < len; i++) {
        printf("\nSending byte: %c (0x%x)\n", bytestream[i], bytestream[i]);
        transmit_arq_frame((uint8_t)bytestream[i], seq_num);
        seq_num ^= 1;
    }

    printf("[client] Done sending.\n");
    return 0;
}
