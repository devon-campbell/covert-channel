GCC = gcc
CFLAGS = -Wall -Werror -Wpedantic

all: receiver sender threshold benchmark

benchmark: benchmark.c
	$(GCC) $(CFLAGS) $< -o benchmark -O0

threshold: threshold.c
	$(GCC) $(CFLAGS) $< -o threshold -O0 -lm

receiver: receiver.c
	$(GCC) $(CFLAGS) $< -o receiver -O0

sender: sender.c
	$(GCC) $(CFLAGS) $< -o sender -O0

clean:
	rm receiver sender threshold benchmark