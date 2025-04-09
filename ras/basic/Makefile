CC = gcc
CFLAGS = -Wall -Werror -Wpedantic -Wextra
ARQ_HDS = send.h receive.h print_utils.h arq_channel.h
ARQ_OBS = send.o receive.o frame_channel.o
TARGETS = receiver sender threshold benchmark

all: $(TARGETS)

benchmark: benchmark.c
	$(CC) $(CFLAGS) $< -o benchmark -O0

threshold: threshold.c
	$(CC) $(CFLAGS) $< -o threshold -O0 -lm

receiver: receiver.c
	$(CC) $(CFLAGS) $< -o receiver -O0

sender: sender.c
	$(CC) $(CFLAGS) $< -o sender -O0

send.h: covert_utils.h
receive.h: covert_utils.h

clean:
	rm $(TARGETS) -f *.o