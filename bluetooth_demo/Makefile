CC = gcc
CCOPTS = -ggdb3 -O0
LIBS = -lbluetooth

all: client server

client: client.c
	$(CC) $(CCOPTS) client.c -o client $(LIBS)

server: server.c
	$(CC) $(CCOPTS) server.c -o server $(LIBS)

clean:
	rm -rf client server
