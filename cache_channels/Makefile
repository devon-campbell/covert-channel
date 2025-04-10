

default: all

# Common compiler flags
CFLAGS = -Wall -Wextra -Wno-nonnull -Wno-unused-variable

# Define sources and include paths
CHANNEL_HEADERS = utils.h bit_channel.h arq_frame.h arq_channel.h
PROTOCOL_HEADERS = utils.h arq_protocol.h arq_channel.h

# BIT_DIR = flush_reload
# BIT_OFILE = $(BIT_DIR)/fr_bit_channel.o
# BIT_DIR = prime_probe
# BIT_OFILE = $(BIT_DIR)/pp_bit_channel.o

# # Build all targets
# all: arq_channel.o arq_protocol.o server client benchmark benchmark_sendbit
# Build all targets
all: flush_reload

# Target to build everything with flush_reload
flush_reload: clean
	$(MAKE) BIT_DIR=flush_reload BIT_OFILE=flush_reload/fr_bit_channel.o server client benchmark benchmark_sendbit

# Target to build everything with prime_probe
prime_probe: clean
	$(MAKE) BIT_DIR=prime_probe BIT_OFILE=prime_probe/pp_bit_channel.o server client benchmark benchmark_sendbit

.PHONY: all flush_reload_build prime_probe_build

log.o: log.c log.h
	$(CC) $(CFLAGS) -c log.c -o log.o

# Rule for arq_channel.o
arq_channel.o: arq_channel.c $(CHANNEL_HEADERS) $(BIT_OFILE) log.o
	$(MAKE) -C $(BIT_DIR)
	$(CC) $(CFLAGS) -c arq_channel.c -o arq_channel.o log.o

# Rule for arq_protocol.o
arq_protocol.o: arq_protocol.c $(PROTOCOL_HEADERS) arq_channel.o log.o
	$(CC) $(CFLAGS) -c arq_protocol.c -o arq_protocol.o log.o

# Rule for server executable
server: server.c arq_protocol.o arq_channel.o log.o
	$(CC) $(CFLAGS) server.c arq_protocol.o arq_channel.o $(BIT_OFILE) log.o -o server

# Rule for client executable
client: client.c arq_protocol.o arq_channel.o
	$(CC) $(CFLAGS) client.c arq_protocol.o arq_channel.o $(BIT_OFILE) log.o -o client

benchmark: benchmark.c arq_protocol.o arq_channel.o
	$(CC) $(CFLAGS) benchmark.c arq_protocol.o arq_channel.o $(BIT_OFILE) log.o -o benchmark

benchmark_sendbit: benchmark_sendbit.c arq_protocol.o arq_channel.o
	$(CC) $(CFLAGS) benchmark_sendbit.c arq_protocol.o arq_channel.o $(BIT_OFILE) log.o -o benchmark_sendbit


# Clean up
clean:
	$(MAKE) -C prime_probe clean
	$(MAKE) -C flush_reload clean
	rm -f *.o server client benchmark benchmark_sendbit

.PHONY: all clean
