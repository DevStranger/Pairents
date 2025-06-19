#include "creature.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>

void init_creature(Creature *c) {
    c->hunger = 80;
    c->happiness = 70;
    c->sleep = 100;
    c->health = 85;
    c->growth = 25;
    c->love = 50;
    c->level = 0;

    time_t now = time(NULL);
    c->last_update1 = now;
    c->last_update2 = now;
    c->last_update3 = now;
    c->last_update4 = now;

    c->ascii_art = NULL;
    c->temp_ascii_art = NULL;
    c->temp_art_end_time = 0;
}

void update_creature(Creature *c) {
    time_t now = time(NULL);

    if (difftime(now, c->last_update1) >= 480) { 
        if (c->hunger > 0) c->hunger--;
        c->last_update1 = now;
    }

    if (difftime(now, c->last_update2) >= 900) {
        if (c->happiness > 0) c->happiness--;
        if (c->love > 0 && c->growth < 30) c->love--;
        c->last_update2 = now;
    }

    if (difftime(now, c->last_update3) >= 600) {
        if (c->sleep > 0) c->sleep--;
        c->last_update3 = now;
    }

    if (difftime(now, c->last_update4) >= 1200) {
        if (c->growth > 0) c->growth--;
        if (c->hunger < 30 && c->health > 0) c->health--;
        c->last_update4 = now;
    }

    if (c->hunger > 70 && c->happiness > 85 && c->growth > 80 && c->love > 99) {
        c->level++;
    }
}

void apply_action(Creature *c, unsigned char action) {
    switch (action) {
        case 0: // Fed
            c->hunger += 15;
            if (c->hunger > 100) c->hunger = 100;
            break;
        case 1: // Read
            c->growth += 10;
            if (c->growth > 100) c->growth = 100;
            break;
        case 2: // Slept
            c->sleep += 20;
            if (c->sleep > 100) c->sleep = 100;
            break;
        case 3: // Hugged
            c->love += 15;
            if (c->love > 100) c->love = 100;
            break;
        case 4: // Played
            c->happiness += 15;
            if (c->happiness > 100) c->happiness = 100;
            break;
        default:
            break;
    }
}

void set_temp_ascii_art(Creature *c, char *new_art, Uint32 duration_ms) {
    if (!new_art) {
        fprintf(stderr, "set_temp_ascii_art: nie podano ASCII artu\n");
        return;
    }

    if (c->ascii_art)
        free(c->ascii_art);

    c->ascii_art = new_art;
    c->temp_art_end_time = SDL_GetTicks() + duration_ms;
}

// --- Obsługa odbioru struktury Creature po kawałku ---

typedef struct {
    char recv_buffer[sizeof(Creature)];
    size_t received_bytes;
    int receiving;
} CreatureReceiver;

static CreatureReceiver receiver = { .received_bytes = 0, .receiving = 0 };

void reset_creature_receiver(void) {
    receiver.received_bytes = 0;
    receiver.receiving = 0;
}

// Zwraca:
// 1 - odebrano całość struktury,
// 0 - nadal czekamy na dane,
// -1 - błąd (recv zwrócił błąd lub zamknięcie połączenia)
int creature_recv_partial(int sock, Creature *c) {
    if (!receiver.receiving) {
        receiver.receiving = 1;
        receiver.received_bytes = 0;
    }

    ssize_t n = recv(sock, receiver.recv_buffer + receiver.received_bytes,
                     sizeof(Creature) - receiver.received_bytes, 0);

    if (n == -1) {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
            return 0; // brak danych teraz, spróbuj później
        perror("recv");
        return -1;
    } else if (n == 0) {
        fprintf(stderr, "Serwer zamknął połączenie\n");
        return -1;
    }

    receiver.received_bytes += n;

    if (receiver.received_bytes >= sizeof(Creature)) {
        memcpy(c, receiver.recv_buffer, sizeof(Creature));
        reset_creature_receiver();
        return 1;
    }

    return 0;
}
