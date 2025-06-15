#ifndef CREATURE_H
#define CREATURE_H

#include <time.h>
#include <pthread.h>  // dodajemy, bo deklarujemy mutex

// Struktura stanu stworzenia
typedef struct {
    int hunger;     // 0 - głodny, 100 - najedzony
    int happiness;  // 0 - smutny, 100 - szczęśliwy
    int sleep;      // 0 - niewyspany, 100 - wyspany
    int health;     // 0 - chory, 100 - zdrowy
    int growth;     // 0 - brak rozwoju, 100 - rozwinięty
    int love;       // 0 - samotny, 100 - kochany i kochający
    int level;      // ogólny poziom gry

    time_t last_update1;
    time_t last_update2;
    time_t last_update3;
    time_t last_update4;
} Creature;

// Globalna instancja stworzenia (deklaracja)
extern Creature current_creature;

// Mutex do synchronizacji dostępu do stworzenia (deklaracja)
extern pthread_mutex_t creature_lock;

// Inicjalizacja stworzenia (ustawia wartości początkowe i timery)
void init_creature(void);

// Cycliczna aktualizacja stanu na podstawie czasu
void update_creature_time(void);

// Aktualizacja na podstawie akcji (0-Feed, 1-Read, 2-Sleep, 3-Hug, 4-Play)
void update_creature_action(int action);

#endif
