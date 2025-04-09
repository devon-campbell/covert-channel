# Makefile -------------------------------------------------------------
CC = gcc
CFLAGS = -O2 -Wall -std=c11
LDLIBS =

all: arq_client arq_server

arq_client: arq_client.c arq_channel.c phy.c pp_common.c covert_utils.h
	$(CC) $(CFLAGS) $^ -o arq_client $(LDLIBS)

arq_server: arq_server.c arq_channel.c phy.c pp_common.c covert_utils.h
	$(CC) $(CFLAGS) $^ -o arq_server $(LDLIBS)

clean:
	rm -f arq_client arq_server sender receiver *.o client_sent.txt server_recv.txt
