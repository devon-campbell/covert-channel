CC = gcc
CFLAGS = -Wall -Wextra -Wno-nonnull -Wno-unused-variable -lm
ARQ_OBS = send.o receive.o frame_channel.o covert_utils.o
TARGETS = arq_client arq_server
TEST = send_bitstream receive_bitstream

all: $(TARGETS) $(TEST)

test: $(TEST)

arq_client: $(ARQ_OBS)
	$(CC) $(CFLAGS) $(ARQ_OBS) -O3 arq_client.c -o arq_client

arq_server: $(ARQ_OBS)
	$(CC) $(CFLAGS) $(ARQ_OBS) -O3 arq_server.c -o arq_server

send_bitstream: send_bitstream.o send.o send_bitstream.c 
	$(CC) $(CFLAGS) send_bitstream.c send.o -o send_bitstream

receive_bitstream:  receive_bitstream.o receive.o receive_bitstream.c
	$(CC) $(CFLAGS) receive_bitstream.c receive.o -o receive_bitstream

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *.txt $(TARGETS)