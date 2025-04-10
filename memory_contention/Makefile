CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lrt

# Object files
OBJS = utils.o

# Executables
EXECS = test test1

all: $(EXECS)

# Main executables
test: test.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test1: test1.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

sender: sender.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

receiver: receiver.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
# Object files
utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c -o $@ $<

test.o: test.c utils.h
	$(CC) $(CFLAGS) -c -o $@ $<

test1.o: test1.c utils.h
	$(CC) $(CFLAGS) -c -o $@ $<

test1.o: test1.c utils.h
	$(CC) $(CFLAGS) -c -o $@ $<

sender.o: sender.c utils.h
	$(CC) $(CFLAGS) -c -o $@ $<

receiver.o: receiver.c utils.h
	$(CC) $(CFLAGS) -c -o $@ $<


clean:
	rm -f $(EXECS) *.o

.PHONY: all clean