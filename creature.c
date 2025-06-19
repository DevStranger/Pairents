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

    // Zapamiętujemy poprzedni stan
    int old_hunger = c->hunger;
    int old_happiness = c->happiness;
    int old_sleep = c->sleep;
    int old_health = c->health;
    int old_growth = c->growth;
    int old_love = c->love;
    int old_level = c->level;

    // Logika aktualizacji
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

    // Sprawdzenie, czy coś się zmieniło
    if (c->hunger != old_hunger ||
        c->happiness != old_happiness ||
        c->sleep != old_sleep ||
        c->health != old_health ||
        c->growth != old_growth ||
        c->love != old_love ||
        c->level != old_level) {

        c->changed = 1;
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
