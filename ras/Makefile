GCC = gcc
CFLAGS = -Wall -Werror -Wpedantic -O0

all: receiver sender threshold benchmark

benchmark: benchmark.c
	$(GCC) $(CFLAGS) $< -o benchmark

threshold: threshold.c
	$(GCC) $(CFLAGS) $< -o threshold -lm

receiver: receiver.c
	$(GCC) $(CFLAGS) $< -o receiver

sender: sender.c
	$(GCC) $(CFLAGS) $< -o sender

clean:
	rm receiver sender threshold benchmark