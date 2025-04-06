

default: all

# Common compiler flags
CFLAGS = -Wall -Wextra -Wno-nonnull -Wno-unused-variable -lm

# threshold: threshold.c channel.h
# 	gcc $(CFLAGS) -o threshold threshold.c

# sender: sender.c channel.h
# 	gcc $(CFLAGS) -o sender sender.c

# receiver: receiver.c channel.h
# 	gcc $(CFLAGS) -o receiver receiver.c

send.h: covert_utils.h
receive.h: covert_utils.h

send.o: send.h
	gcc $(CFLAGS) -c send.c

receive.o: receive.h
	gcc $(CFLAGS) -c receive.c

frame_channel.o: frame_channel.c send.h receive.h
	gcc $(CFLAGS) -c frame_channel.c send.o receive.o

send_bitstream: send_bitstream.c print_utils.h send.h send.o
	gcc $(CFLAGS) -o send_bitstream send_bitstream.c send.o

receive_bitstream: receive_bitstream.c print_utils.h receive.h receive.o
	gcc $(CFLAGS) -o receive_bitstream receive_bitstream.c receive.o

send_bytestream: send_bytestream.c print_utils.h send.h send.o
	gcc $(CFLAGS) -o send_bytestream send_bytestream.c send.o

receive_bytestream: receive_bytestream.c print_utils.h receive.h receive.o
	gcc $(CFLAGS) -o receive_bytestream receive_bytestream.c receive.o

arq_client: arq_client.c print_utils.h send.h receive.h send.o receive.o frame_channel.o
	gcc $(CFLAGS) -o arq_client arq_client.c send.o receive.o frame_channel.o

arq_server: arq_server.c print_utils.h send.h receive.h send.o receive.o frame_channel.o
	gcc $(CFLAGS) -o arq_server arq_server.c send.o receive.o frame_channel.o

clean:
	rm -f threshold sender receiver *.o receive_bitstream send_bitstream arq_client arq_server send_bytestream receive_bytestream

all: send_bitstream receive_bitstream send_bytestream receive_bytestream arq_client arq_server




