#include "creature.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>

// inicjalizacja stwora
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
}

// aktualizacja stanu stwora na podstawie upływu czasu
void update_creature(Creature *c) {
    time_t now = time(NULL);

    if (difftime(now, c->last_update1) >= 1) {  //było 480 chwilowo 1
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

    // warunki na level up
    if (c->hunger > 70 && c->happiness > 85 && c->growth > 80 && c->love > 99) {
        c->level++;
    }
}

// zmiana stanu stwora ze względu na wybraną akcję
void apply_action(Creature *c, unsigned char action) {
    switch (action) {
        case 0: // Fed (nakarmiony)
            c->hunger += 12;
            if (c->hunger > 100) c->hunger = 100;
            break;
        case 1: // Read (poczytane)
            c->growth += 8;
            if (c->growth > 100) c->growth = 100;
            break;
        case 2: // Slept (wyspany)
            c->sleep += 7;
            if (c->sleep > 100) c->sleep = 100;
            break;
        case 3: // Hugged (przytulony)
            c->love += 12;
            if (c->love > 100) c->love = 100;
            break;
        case 4: // Played (zabawiony)
            c->happiness += 11;
            if (c->happiness > 100) c->happiness = 100;
            break;
        default:
            break;
    }
}

// odbiór struktury stwora po kawałku
typedef struct {
    char recv_buffer[sizeof(Creature)];        // bufor na dane stwora
    size_t received_bytes;                     // ile bajtów już odebrano
    int receiving;                             // flaga czy trwa odbiór czy już nie
} CreatureReceiver;

static CreatureReceiver receiver = { .received_bytes = 0, .receiving = 0 };

// reset stanu odbiornika (koniec/błąd)
void reset_creature_receiver(void) {
    receiver.received_bytes = 0;
    receiver.receiving = 0;
}

// funkcja odbierająca dane o stworku, zwraca:
// 1 - odebrano całość struktury,
// 0 - nadal czekamy na dane,
// -1 - błąd (recv zwrócił błąd lub zamknięcie połączenia)
int creature_recv_partial(int sock, Creature *c) {
    // jeśli nie odbieramy
    if (!receiver.receiving) {
        receiver.receiving = 1;
        receiver.received_bytes = 0;
    }

    // próba odebrania kolejnego fargmentu
    ssize_t n = recv(sock, receiver.recv_buffer + receiver.received_bytes,
                     sizeof(Creature) - receiver.received_bytes, 0);

    if (n == -1) {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
            return 0;             // brak danych teraz, spróbuj później
        perror("recv");
        return -1;
    } else if (n == 0) {
        fprintf(stderr, "Serwer zamknął połączenie\n");
        return -1;
    }

    // aktualizujemy ile bajtów już odebraliśmy
    receiver.received_bytes += n;

    // jeśli odebralismy pełną strukturę, kopiujemy do podanego obiektu i resetujemy odbiornik
    if (receiver.received_bytes >= sizeof(Creature)) {
        memcpy(c, receiver.recv_buffer, sizeof(Creature));
        reset_creature_receiver();
        return 1;                // wszystko odebrane
    }

    return 0;                    // czekamy na kolejne dane
}
