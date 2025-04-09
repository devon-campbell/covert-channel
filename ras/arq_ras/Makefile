CC = gcc
CFLAGS = -Wall -Wextra -Wno-nonnull -Wno-unused-variable -lm -g
ARQ_OBS = send.o receive.o covert_utils.o frame_channel.o ras_time.o
TARGETS = arq_client arq_server

all: $(TARGETS)

arq_client: $(ARQ_OBS) arq_client.c
	$(CC) $(CFLAGS) $(ARQ_OBS) -O3 arq_client.c -o arq_client

arq_server: $(ARQ_OBS) arq_server.c
	$(CC) $(CFLAGS) $(ARQ_OBS) -O3 arq_server.c -o arq_server

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *.txt $(TARGETS)