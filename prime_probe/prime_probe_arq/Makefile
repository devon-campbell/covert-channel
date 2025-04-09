# Makefile -------------------------------------------------------------
CC      = gcc
CFLAGS  = -O2 -Wall -std=c11
LDLIBS  = 

# Common source files
UTILS   = covert_utils.h
PHY     = phy.c pp_common.c
SEND    = send.c
RECV    = receive.c
ARQ     = arq_channel.c
HEADERS = covert_utils.h phy.h send.h receive.h arq_channel.h

# Legacy support (optional)
LEGACY_SENDER   = pp_send.c pp_common.c
LEGACY_RECEIVER = pp_recv.c pp_common.c

# Targets to build everything
all: arq_client arq_server sender receiver

# New ARQ-layer applications
arq_client: arq_client.c arq_channel.c send.c receive.c phy.c pp_common.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

arq_server: arq_server.c arq_channel.c send.c receive.c phy.c pp_common.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

# Optional: Legacy PHY-only test programs
sender: $(LEGACY_SENDER)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

receiver: $(LEGACY_RECEIVER)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

# Cleanup
clean:
	rm -f arq_client arq_server sender receiver *.o
