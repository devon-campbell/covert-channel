#include <stdio.h>
#include <stdlib.h>

int main() {
    FILE *sent = fopen("client_sent.txt", "r");
    FILE *recv = fopen("server_recv.txt", "r");
    FILE *time = fopen("time.txt", "r");

    double elapsed;
    double bps;

    if (fscanf(time, "%lf", &elapsed) < 1) {
        perror("Error opening files");
    }

    if (!sent || !recv) {
        perror("Error opening files");
        return 1;
    }

    int total = 0;
    int matches = 0;
    int mismatches = 0;

    while (1) {
        int c1 = fgetc(sent);
        int c2 = fgetc(recv);

        if (c1 == EOF || c2 == EOF) break;

        if (c1 == c2) {
            matches++;
        } else {
            mismatches++;
            // printf("Mismatch at index %d: sent=0x%02x, recv=0x%02x\n", total, c1, c2);
        }

        total++;
    }

    bps = total / elapsed; 

    fclose(sent);
    fclose(recv);

    printf("=== Transmission Report ===\n");
    printf("Elapsed time: %.3f seconds\n", elapsed);
    printf("Total bytes compared: %d\n", total);
    printf("Bytes per second: %.3f \n", bps);
    printf("Matches: %d\n", matches);
    printf("Mismatches: %d\n", mismatches);
    printf("Accuracy: %.2f%%\n", (total > 0) ? (100.0 * matches / total) : 0.0);

    return 0;
}