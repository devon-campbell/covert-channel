CC = CC
CFLAGS = -Wall -Werror -Wpedantic -Wextra
ARQ_HDS = send.h receive.h print_utils.h arq_channel.h
ARQ_OBS = send.o receive.o frame_channel.o

all: receiver sender threshold benchmark arq_client arq_server

benchmark: benchmark.c
	$(CC) $(CFLAGS) $< -o benchmark -O0

threshold: threshold.c
	$(CC) $(CFLAGS) $< -o threshold -O0 -lm

receiver: receiver.c
	$(CC) $(CFLAGS) $< -o receiver -O0

sender: sender.c
	$(CC) $(CFLAGS) $< -o sender -O0

arq_client: $(ARQ_OBS) $(ARQ_HDS) arq_client.c
	$(CC) $(CFLAGS) arq_client.c $(ARQ_OBS) -o arq_client -O3

arq_server: $(ARQ_OBS) $(ARQ_HDS) arq_server.c
	$(CC) $(CFLAGS) arq_server.c $(ARQ_OBS) -o arq_server -O3

send.h: covert_utils.h
receive.h: covert_utils.h

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<

frame_channel.o: frame_channel.c send.o receive.o
	$(CC) $(CFLAGS) -c frame_channel.c send.o receive.o -O3

clean:
	rm receiver sender threshold benchmark arq_client arq_server -f *.o