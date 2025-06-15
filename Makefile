CC := gcc
CFLAGS := -Wall -Wextra -pthread `sdl2-config --cflags`
LDFLAGS := `sdl2-config --libs` -lSDL2_ttf -pthread

OBJDIR := obj
BINDIR := bin

# Pliki źródłowe
SERVER_SRC := server.c creature.c
CLIENT_SRC := client.c creature.c GUI.c

# Pliki obiektowe (osobne wersje creature.o!)
SERVER_OBJ := $(OBJDIR)/server.o $(OBJDIR)/creature_server.o
CLIENT_OBJ := $(OBJDIR)/client.o $(OBJDIR)/creature_client.o $(OBJDIR)/GUI.o

all: dirs server client

dirs:
	@mkdir -p $(OBJDIR) $(BINDIR)

# Kompilacja plików .c do .o
$(OBJDIR)/server.o: server.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/client.o: client.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/creature_server.o: creature.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/creature_client.o: creature.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/GUI.o: GUI.c
	$(CC) $(CFLAGS) -c $< -o $@

server: $(SERVER_OBJ)
	$(CC) $^ -o $(BINDIR)/server $(LDFLAGS)

client: $(CLIENT_OBJ)
	$(CC) $^ -o $(BINDIR)/client $(LDFLAGS)

clean:
	rm -rf $(OBJDIR) $(BINDIR)

.PHONY: all dirs clean server client

