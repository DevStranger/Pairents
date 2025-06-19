#include "creature.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h> // dla recv()

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

    c->received_bytes = 0;
    c->receiving = 0;
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

// Funkcja odbierająca strukturę Creature w kawałkach (non-blocking recv)
int try_receive_creature(int sock, Creature *c) {
    if (!c->receiving) {
        c->received_bytes = 0;
        c->receiving = 1;
    }

    ssize_t n = recv(sock, c->recv_buffer + c->received_bytes,
                     sizeof(Creature) - c->received_bytes, 0);

    if (n == -1) {
        // Brak danych lub błąd
        return 0;
    } else if (n == 0) {
        // Serwer zamknął połączenie
        fprintf(stderr, "Serwer zamknął połączenie\n");
        return -1;
    }

    c->received_bytes += n;

    if (c->received_bytes >= sizeof(Creature)) {
        // Odbiór zakończony — kopiujemy dane do struktury
        memcpy(c, c->recv_buffer, sizeof(Creature));
        c->receiving = 0;
        return 1; // Sukces — dane gotowe
    }

    return 0; // Nadal czekamy na więcej danych
}
