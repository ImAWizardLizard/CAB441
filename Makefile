CC = gcc
CFLAGS = -Wall 


all: server client
	rm -f *.o

server: server.o utils.o
	$(CC) $(CFLAGS) -o server server.o utils.o

client: client.o utils.o
	$(CC) $(CFLAGS) -o client client.o utils.o
