#include "creature.h"
#include <time.h>

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

void update_creature(Creature *c) {
    time_t now = time(NULL);

    // Hunger co 8 minut (480 sekund)
    if (difftime(now, c->last_update1) >= 480) {
        if (c->hunger > 0) c->hunger -= 1;
        c->last_update1 = now;
    }

    // Happiness i Love co 15 minut (900 sekund)
    if (difftime(now, c->last_update2) >= 900) {
        if (c->happiness > 0) c->happiness -= 1;
        if (c->love > 0 && c->growth < 30) c->love -= 1;
        c->last_update2 = now;
    }

    // Sleep co 10 minut (600 sekund)
    if (difftime(now, c->last_update3) >= 600) {
        if (c->sleep > 0) c->sleep -= 1;
        c->last_update3 = now;
    }

    // Health i Growth co 20 minut (1200 sekund)
    if (difftime(now, c->last_update4) >= 1200) {
        if (c->growth > 0) c->growth -= 1;
        if (c->hunger < 30 && c->health > 0) c->health -= 1;
        c->last_update4 = now;
    }

    // Level-up sprawdzamy bez opóźnienia lub można dodać osobny timer, jeśli chcesz
    if (c->hunger > 70 && c->happiness > 85 && c->growth > 80 && c->love > 99) {
        c->level += 1;
    }
}
