#ifndef CREATURE_H
#define CREATURE_H

#include <time.h>
#include <SDL2/SDL.h>
#include <stdint.h>

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

    char *ascii_art;
    char *temp_ascii_art;
    Uint32 temp_art_end_time;
} Creature;

void init_creature(Creature *c);
void update_creature(Creature *c);
void set_temp_ascii_art(Creature *c, char *new_art, Uint32 duration_ms);

void reset_creature_receiver(void);
int creature_recv_partial(int sock, Creature *c);

#endif // CREATURE_H
