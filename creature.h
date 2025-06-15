#ifndef CREATURE_H
#define CREATURE_H

#include <time.h>

typedef struct {
    int hunger;
    int happiness;
    int sleep;
    int health;
    int growth;
    int love;
    int level;

    time_t last_update1;
    time_t last_update2;
    time_t last_update3;
    time_t last_update4;

    const char *ascii_art;
} Creature;

void init_creature(Creature *c);
void update_creature(Creature *c);

#endif // CREATURE_H
