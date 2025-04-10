# Compiler
CC = gcc

# Compiler flags
CFLAGS = -O0 -Wall -Wextra -Wno-unused-variable -g

# Header dependencies
HDRS = ../bit_channel.h

# Default target
all: $(OBJ)

# Rule to compile .c into .o with header dependency
pp_bit_channel.o: pp_bit_channel.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target
clean:
	rm -f $(OBJ)

# Phony targets
.PHONY: all clean