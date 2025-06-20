# Kompilator i flagi kompilacji
CC = gcc
CFLAGS = -Wall -I game $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs)

# Pliki obiektowe
CLIENT_OBJS = client.o gui.o creature.o
SERVER_OBJS = server.o creature.o

all: client server

client: $(CLIENT_OBJS)
	$(CC) $(CLIENT_OBJS) -o game/client $(LDFLAGS)

server: $(SERVER_OBJS)
	$(CC) $(SERVER_OBJS) -o game/server $(LDFLAGS)

client.o: game/client.c
	$(CC) $(CFLAGS) -c game/client.c -o client.o

gui.o: game/gui.c
	$(CC) $(CFLAGS) -c game/gui.c -o gui.o

creature.o: game/creature.c
	$(CC) $(CFLAGS) -c game/creature.c -o creature.o

server.o: game/server.c
	$(CC) $(CFLAGS) -c game/server.c -o server.o

clean:
	rm -f $(CLIENT_OBJS) $(SERVER_OBJS) game/client game/server

.PHONY: all client server clean
