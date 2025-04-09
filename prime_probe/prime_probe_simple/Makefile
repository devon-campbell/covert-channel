# Makefile -------------------------------------------------------------
CC=gcc
CFLAGS=-O2 -Wall -std=c11
LDLIBS=

all: sender receiver

sender: pp_send.c pp_common.c
	$(CC) $(CFLAGS) $^ -o sender $(LDLIBS)

receiver: pp_recv.c pp_common.c
	$(CC) $(CFLAGS) $^ -o receiver $(LDLIBS)
