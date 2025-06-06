#ifndef CREATURE_H
#define CREATURE_H

typedef struct {
    int hunger;     // 0-glodny, 100-najedzony
    int happiness;  // 0-smutny, 100-szczesliwy
    int sleep;      // 0-niewyspany, 100-wyspany
    int health;     // 0-chory, 100-zdrowy
    int growth;     // 0-brak rozwoju, 100-rozwiniety
    int love;       // 0-samotny, 100-kochany i kochajacy
    int level;      // ogolny poziom gry
} Creature;

void init_creature(Creature *c);
void update_creature(Creature *c, float delta_seconds);

#endif
