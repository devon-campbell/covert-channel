CC = gcc
CFLAGS = -Wall -Wextra -Wno-nonnull -Wno-unused-variable -lm -g -O0
ARQ_OBS = send.o receive.o covert_utils.o frame_channel.o ras_time.o
TARGETS = arq_client arq_server
TEST = send_bitstream receive_bitstream

all: $(TARGETS) $(TEST)

test: $(TEST)

arq_client: $(ARQ_OBS) arq_client.c
	$(CC) $(CFLAGS) $(ARQ_OBS) arq_client.c -o arq_client

arq_server: $(ARQ_OBS) arq_server.c
	$(CC) $(CFLAGS) $(ARQ_OBS) arq_server.c -o arq_server

send_bitstream: send_bitstream.o send.o send_bitstream.c 
	$(CC) $(CFLAGS) send_bitstream.c send.o -o send_bitstream

receive_bitstream:  receive_bitstream.o receive.o ras_time.o receive_bitstream.c
	$(CC) $(CFLAGS) receive_bitstream.c receive.o ras_time.o -o receive_bitstream

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $< -O0


clean:
	rm -f *.o *.txt $(TARGETS) $(TEST)