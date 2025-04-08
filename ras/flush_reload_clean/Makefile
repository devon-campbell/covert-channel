CC = gcc
CFLAGS = -Wall -Wextra -Wno-nonnull -Wno-unused-variable -lm
ARQ_OBS = send.o receive.o covert_utils.o frame_channel.o
TARGETS = arq_client arq_server

all: $(TARGETS)

arq_client: $(ARQ_OBS)
	$(CC) $(CFLAGS) $(ARQ_OBS) -O3 arq_client.c -o arq_client

arq_server: $(ARQ_OBS)
	$(CC) $(CFLAGS) $(ARQ_OBS) -O3 arq_server.c -o arq_server

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *.txt $(TARGETS)