#define _GNU_SOURCE
#include "arq_protocol.h"
#include "arq_channel.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sched.h>

void calibrate_transmission()
{
    calibrate_frame_channel();
}

// Sender (outgoing) address for server, receiver (incoming) address for client
static void *client_address = NULL;
static inline void *get_client_address()
{
    if (client_address != NULL)
    {
        return client_address;
    }
    client_address = shared_file_address("/bin/ls", 0);
    return client_address;
}

// Sender (outgoing) address for server, receiver (incoming) address for client
static void *server_address = NULL;
static inline void *get_server_address()
{
    if (server_address != NULL)
    {
        return server_address;
    }
    server_address = shared_file_address("/bin/tmux", 0);
    return server_address;
}


static inline uint8_t next_seq_num(uint8_t seq_num)
{
    // return (seq_num + 1) % 4;
    return (seq_num + 1) % 2;
}

// ARQ server waits for messages from client
int arq_server(char *log_fname, size_t measure_interval)
{
    void *out_addr = get_client_address();
    void *in_addr = get_server_address();
    if (out_addr == NULL || in_addr == NULL)
    {
        perror("Error: Could not shared addresses.\n");
        return 1;
    }

    info("[arq_server] Opening log file: %s\n", log_fname);
    FILE *log_file = fopen(log_fname, "w");
    if (log_file == NULL)
    {
        perror("Failed to open log file");
        return 1;
    }
    FILE *timestamp_file = fopen(SERVER_TIMESTAMP_FILE, "w");
    if (timestamp_file == NULL)
    {
        perror("Failed to open log file");
        return 1;
    }

    info("[arq_server] Calibrating...\n");
    calibrate_transmission();
    uint8_t seq_num = 0; // increments mod 4
    arq_frame_t frame_buf;
    int char_count = 0;
    while (1)
    {
        arq_frame_t *ret = receive_arq_frame(in_addr, out_addr, &frame_buf, seq_num);
        if (ret == NULL)
        {
            fprintf(stderr, "Failed to receive a valid frame.\n");
            continue; // Retry receiving
        }

        char_count++;

        if (char_count % measure_interval == 0)
        {
            // Write timestamp to file
            uint64_t timestamp = get_usec();
            if (timestamp_file != NULL)
            {
                fprintf(timestamp_file, "end,%d,%lu\n", char_count, timestamp);
                fflush(timestamp_file);
            }
        }

        // Extract data
        uint8_t byte = frame_data(&frame_buf);
        info("Received byte:%c (0x%x) (With seq num %d)\n", byte, byte, seq_num);
        fprintf(stderr, "[server] Received byte:%c (0x%x) (With seq num %d)\n", byte, byte, seq_num);

        seq_num = next_seq_num(seq_num);

        fputc(byte, log_file);
        fflush(log_file);

#ifdef SINGLE_CLIENT_MESSAGE
        if (char_count % measure_interval == 0)
        {
            break;
        }
#endif
    }

    fclose(log_file);
    fclose(timestamp_file);
    info("[arq_server] Done receiving.\n");
    return 0;
}


void transmit_message(char *message, void *out_addr, void *in_addr)
{
    uint8_t seq_num = 0; // toggles between 0 and 1
    char *curr_byte = message;
    while (*curr_byte)
    {
        info("\nSending byte: %c (0x%x) (With seq num %d)\n", *curr_byte, *curr_byte, seq_num);
        fprintf(stderr, "\nSending byte: %c (0x%x) (With seq num %d)\n", *curr_byte, *curr_byte, seq_num);

        transmit_arq_frame(out_addr, in_addr, (uint8_t)*curr_byte, seq_num, FRAME_TIMEOUT, FRAME_RETRIES);
        seq_num = next_seq_num(seq_num); // Toggle sequence number for next frame

        curr_byte++;
    }
}

int arq_client(char *message, const char *log_fname)
{
    void *out_addr = get_server_address();
    void *in_addr = get_client_address();
    if (out_addr == NULL || in_addr == NULL)
    {
        perror("Error: Could not shared addresses.\n");
        return 1;
    }

    FILE *log_file = fopen(log_fname, "w");
    if (log_file == NULL)
    {
        perror("Failed to open log file");
        return 1;
    }

    fprintf(log_file, "%s", message);
    fclose(log_file);

    calibrate_transmission();

    // Repeatedly send message then sleep
    uint64_t start_time = get_usec();
    uint64_t end_time = 0;
    while (1)
    {
        transmit_message(message, out_addr, in_addr);
#ifdef SINGLE_CLIENT_MESSAGE
        end_time = get_usec();
        break;
#endif
        usleep(MESSAGE_SLEEP_USEC);
    }

    FILE *timestamp_file = fopen(CLIENT_TIMESTAMP_FILE, "w");
    if (timestamp_file == NULL)
    {
        perror("Failed to open log file");
        return 1;
    }

    fprintf(timestamp_file, "start,0,%lu\n", start_time);
    fprintf(timestamp_file, "end,%ld,%lu\n", strlen(message), end_time);
    fclose(timestamp_file);
    info("[arq_client] Done sending.\n");
    return 0;
}

struct time_event
{
    char name[50];
    int char_index;
    uint64_t timestamp;
};

struct time_event *read_time_events(const char *filename, int *num_events)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Failed to open file");
        return NULL;
    }

    struct time_event *events = malloc(100 * sizeof(struct time_event));
    if (events == NULL)
    {
        perror("Failed to allocate memory");
        fclose(file);
        return NULL;
    }

    int count = 0;
    while (fscanf(file, "%[^,],%d,%lu\n", events[count].name, &events[count].char_index, &events[count].timestamp) == 3)
    {
        count++;
        if (count % 100 == 0)
        {
            events = realloc(events, (count + 100) * sizeof(struct time_event));
            if (events == NULL)
            {
                perror("Failed to reallocate memory");
                fclose(file);
                return NULL;
            }
        }
    }

    fclose(file);
    *num_events = count;
    return events;
}

int protocol_benchmark(char *message, char *server_log_fname, char *client_log_fname)
{

    // Delete log and timestamp files
    int old_errno = errno;
    remove(server_log_fname);
    remove(client_log_fname);
    remove(SERVER_TIMESTAMP_FILE);
    remove(CLIENT_TIMESTAMP_FILE);
    errno = old_errno;

    int measure_interval = strlen(message);

    printf("[protocol_benchmark] Starting ARQ protocol benchmark...\n");
    // Launch server in child process
    pid_t server_pid = fork();
    if (server_pid == 0)
    {
        // Child process: pin to CPU 1
        cpu_set_t server_mask;
        CPU_ZERO(&server_mask);
        CPU_SET(1, &server_mask);
        if (sched_setaffinity(0, sizeof(cpu_set_t), &server_mask) < 0)
        {
            perror("Failed to set CPU affinity for server");
            exit(1);
        }

        // Open dumplog files
        FILE *server_debug = fopen("server_debug.log", "w");
        if (server_debug == NULL)
        {
            perror("Failed to open server debug log file");
            return 1;
        }
        set_log_fd(server_debug);

        // Child process: run the server
        printf("[protocol_benchmark] Starting ARQ server on CPU 1...\n");
        arq_server(server_log_fname, measure_interval);
        exit(0);
    }
    else if (server_pid < 0)
    {
        perror("Failed to fork server process");
        return 1;
    }

    FILE *client_debug = fopen("client_debug.log", "w");
    if (client_debug == NULL)
    {
        perror("Failed to open client debug log file");
        fclose(client_debug);
        return 1;
    }
    
    set_log_fd(client_debug);
    // Parent process: pin to CPU 0
    cpu_set_t client_mask;
    CPU_ZERO(&client_mask);
    CPU_SET(0, &client_mask);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &client_mask) < 0)
    {
        perror("Failed to set CPU affinity for client");
        return 1;
    }
    printf("[protocol_benchmark] Parent process running on CPU 0\n");

    printf("[protocol_benchmark] Server PID: %d\n", server_pid);
    // Sleep to allow server to start
    sleep(1);

    // Parent process: run the client, send message, wait for server to finish, then read times
    arq_client(message, client_log_fname);
    printf("[protocol_benchmark] Client finished sending message.\n");
    sleep(1);

    // Wait for server to finish
    int status;
    printf("[protocol_benchmark] Waiting for server to finish (timeout: 10 seconds)...\n");

    // Wait for server with timeout
    int wait_time = 0;
    bool server_finished = false;
    while (wait_time < 10)
    {
        pid_t result = waitpid(server_pid, &status, WNOHANG);
        if (result == server_pid)
        {
            server_finished = true;
            break;
        }
        else if (result < 0)
        {
            perror("waitpid error");
            break;
        }
        sleep(1);
        wait_time++;
    }

    if (!server_finished)
    {
        printf("[protocol_benchmark] Server timeout reached, killing server process...\n");
        kill(server_pid, SIGTERM);
        waitpid(server_pid, &status, 0);
    }

    if (WIFEXITED(status))
    {
        printf("Server exited with status %d\n", WEXITSTATUS(status));
    }
    else if (WIFSIGNALED(status))
    {
        printf("Server terminated by signal %d\n", WTERMSIG(status));
    }
    else
    {
        printf("Server terminated abnormally\n");
    }

    printf("[protocol_benchmark] Server finished, calculating stats\n");
    // Read and print the timestamps
    int num_server_events = 0;
    int num_client_events = 0;
    struct time_event *server_events = read_time_events(SERVER_TIMESTAMP_FILE, &num_server_events);
    struct time_event *client_events = read_time_events(CLIENT_TIMESTAMP_FILE, &num_client_events);
    if (server_events == NULL || client_events == NULL)
    {
        printf("Failed to read time events\n");
        free(server_events);
        free(client_events);
        return 1;
    }
    uint64_t client_start, client_end, server_end = 0;
    int sent_chars = -1;
    int received_chars = -1;
    printf("Server events (%d):\n", num_server_events);
    for (int i = 0; i < num_server_events; i++)
    {
        printf("%s, %d, %lu\n", server_events[i].name, server_events[i].char_index, server_events[i].timestamp);

        if (strcmp(server_events[i].name, "end") == 0)
        {
            server_end = server_events[i].timestamp;
            received_chars = server_events[i].char_index;
        }
    }
    printf("Client events (%d):\n", num_client_events);
    for (int i = 0; i < num_client_events; i++)
    {
        printf("%s, %d, %lu\n", client_events[i].name, client_events[i].char_index, client_events[i].timestamp);
        if (strcmp(client_events[i].name, "start") == 0)
        {
            client_start = client_events[i].timestamp;
        }
        else if (strcmp(client_events[i].name, "end") == 0)
        {
            client_end = client_events[i].timestamp;
            sent_chars = client_events[i].char_index;
        }
    }

    // Calculate time interval
    uint64_t client_time = client_end - client_start;
    uint64_t server_time = server_end - client_start;
    printf("Client duration: %lu microseconds\n", client_time);
    printf("Server duration: %lu microseconds\n", server_time);

    printf("Sent characters: %d\n", sent_chars);
    printf("Received characters: %d\n", received_chars);

    // Calculate throughput (bits per second)
    double client_throughput = (double)sent_chars * 8 / (client_time / 1000000.0);
    double server_throughput = (double)received_chars * 8 / (server_time / 1000000.0);
    printf("Client throughput: %.2f bps\n", client_throughput);
    printf("Server throughput: %.2f bps\n", server_throughput);

    // Read sent and recieved messages from client,s erver logs
    FILE *client_log_file = fopen(client_log_fname, "r");
    FILE *server_log_file = fopen(server_log_fname, "r");
    if (client_log_file == NULL || server_log_file == NULL)
    {
        perror("Failed to open log files");
        free(server_events);
        free(client_events);
        return 1;
    }
    char *client_message = malloc(1000);
    char *server_message = malloc(1000);
    if (client_message == NULL || server_message == NULL)
    {
        perror("Failed to allocate memory for messages");
        fclose(client_log_file);
        fclose(server_log_file);
        free(server_events);
        free(client_events);
        return 1;
    }

    size_t client_len = fread(client_message, 1, 1000, client_log_file);
    size_t server_len = fread(server_message, 1, 1000, server_log_file);
    client_message[client_len] = '\0';
    server_message[server_len] = '\0';
    printf("Client message: %s\n", client_message);
    printf("Server message: %s\n", server_message);

    // Compare lengths
    if (client_len != server_len)
    {
        printf("Mismatch in message lengths: client %zu, server %zu\n", client_len, server_len);
    }
    else
    {
        printf("Message lengths match: %zu bytes\n", client_len);
    }

    // Calculate percent accuracy
    int correct_chars = 0;
    for (size_t i = 0; i < client_len; i++)
    {
        if (client_message[i] == server_message[i])
        {
            correct_chars++;
        }
    }
    double accuracy = (double)correct_chars / client_len * 100.0;
    printf("Accuracy: %.2f%%\n", accuracy);

    // Calculate bit accuracy
    int bit_correct = 0;
    for (size_t i = 0; i < client_len; i++)
    {
        uint8_t client_byte = client_message[i];
        uint8_t server_byte = server_message[i];
        for (int j = 0; j < 8; j++)
        {
            if ((client_byte & (1 << j)) == (server_byte & (1 << j)))
            {
                bit_correct++;
            }
        }
    }

    double bit_accuracy = (double)bit_correct / (client_len * 8) * 100.0;
    printf("Bit accuracy: %.2f%%\n", bit_accuracy);



    // Append message name, message length, server throughput, byte accuracy, and bit accuracy to a CSV file 
    // Open the CSV file or create it with headers if it doesn't exist
    FILE *benchmark_csv = fopen("benchmark_results.csv", "a+");
    if (benchmark_csv == NULL) {
        perror("Failed to open benchmark results file");
    } else {
        // Check if file is empty (new) and add headers if needed
        fseek(benchmark_csv, 0, SEEK_END);
        long size = ftell(benchmark_csv);
        if (size == 0) {
            fprintf(benchmark_csv, "message_file,length,throughput_bps,byte_accuracy_percent,bit_accuracy_percent\n");
        }
        
        // Add the new benchmark entry
        fprintf(benchmark_csv, "%s,%zu,%.2f,%.2f,%.2f\n", 
                client_log_fname, client_len, server_throughput, accuracy, bit_accuracy);
        
        fclose(benchmark_csv);
        printf("Benchmark results saved to benchmark_results.csv\n");
    }




    return 0;
}