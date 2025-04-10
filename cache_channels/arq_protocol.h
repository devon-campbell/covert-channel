#ifndef ARQ_PROTOCOL_H
#define ARQ_PROTOCOL_H
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

// To tune: start at a high value (like 25 * 1000) and decrease until accuracy drops off
// or throughput decreases
#define FRAME_TIMEOUT (25 * 100)  // microseconds

// This shouldn't make a huge difference but tuning can trade off between accuracy and tail latency
#define FRAME_RETRIES 25

// Calibration, e.g. calculate threshold 
void calibrate_transmission(void);


/* ADDRESS COORDINATION */

// // Sender (outgoing) address for server, receiver (incoming) address for client
// inline void *get_client_address();
// // Sender (outgoing) address for server, receiver (incoming) address for client
// inline void *get_server_address();



/* TOP LEVEL WRAPPERS */

// Sleep interval before client repeats message
#define MESSAGE_SLEEP_USEC 1000000 
#define SINGLE_CLIENT_MESSAGE

// Writes current timestamp to timestamp file after 'measure_interval' bytes
#define SERVER_TIMESTAMP_FILE "server_timestamp.txt"
#define CLIENT_TIMESTAMP_FILE "client_timestamp.txt"
int arq_server(char *log_fname, size_t measure_interval);
int arq_client(char* message, const char *log_fname);

int protocol_benchmark(char *message, char *server_log_fname, char *client_log_fname);

#endif // ARQ_PROTOCOL_H