#include "creature.h"

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
