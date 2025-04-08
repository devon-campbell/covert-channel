CC = gcc
CFLAGS = -Wall -Wextra -O0
LDFLAGS =

SRCS = utils.c receiver.c sender.c
OBJS = $(SRCS:.c=.o)

all: receiver sender

receiver: utils.o receiver.o
	$(CC) $(CFLAGS) -o $@ utils.o receiver.o $(LDFLAGS)

sender: utils.o sender.o
	$(CC) $(CFLAGS) -o $@ utils.o sender.o $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) receiver sender

.PHONY: all clean