CC := gcc
CFLAGS := -I./common -I./common/cJSON -Wall -Wextra -pthread
LDFLAGS := -lSDL2 -lSDL2_ttf -pthread

SRCDIRS := server/src client/src common/cJSON
OBJDIR := obj
BINDIR := bin

SERVER_SRC := server/src/main.c 
CLIENT_SRC := client/src/main.c client/src/creature.c
COMMON_SRC := common/cJSON/cJSON.c common/protocol.c

SERVER_OBJ := $(OBJDIR)/server_main.o
CLIENT_OBJ := $(OBJDIR)/client_main.o
COMMON_OBJ := $(OBJDIR)/cJSON.o $(OBJDIR)/protocol.o

all: dirs server client

dirs:
	@mkdir -p $(OBJDIR) $(BINDIR)

$(OBJDIR)/%.o: common/cJSON/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/protocol.o: common/protocol.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/server_main.o: $(SERVER_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/client_main.o: client/src/main.c
	$(CC) $(CFLAGS) -c $< -o $@
	
$(OBJDIR)/client_creature.o: client/src/creature.c
	$(CC) $(CFLAGS) -c $< -o $@

server: $(SERVER_OBJ) $(COMMON_OBJ)
	$(CC) $^ -o $(BINDIR)/server $(LDFLAGS)

client: $(OBJDIR)/client_main.o $(OBJDIR)/client_creature.o $(COMMON_OBJ)
	$(CC) $^ -o $(BINDIR)/client $(LDFLAGS)

clean:
	rm -rf $(OBJDIR) $(BINDIR)

.PHONY: all dirs clean server client
