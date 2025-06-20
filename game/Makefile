CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I.
SDL_CFLAGS = $(shell sdl2-config --cflags)
SDL_LIBS = $(shell sdl2-config --libs) -lSDL2_ttf
THREAD_LIBS = -pthread

# Pliki obiektowe klienta i serwera
CLIENT_OBJS = client.o gui.o creature.o
SERVER_OBJS = server.o creature.o

.PHONY: all clean

all: client server

client: $(CLIENT_OBJS)
	$(CC) $(CLIENT_OBJS) $(SDL_LIBS) $(THREAD_LIBS) -o client

server: $(SERVER_OBJS)
	$(CC) $(SERVER_OBJS) $(SDL_LIBS) $(THREAD_LIBS) -o server

%.o: %.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

clean:
	rm -f *.o client server
