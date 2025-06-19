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

    char *ascii_art;          // domyślny ASCII art
    char *temp_ascii_art;     // tymczasowy ASCII art po akcji
    Uint32 temp_art_end_time; // czas kiedy przestajemy wyświetlać temp ascii art (SDL_GetTicks ms)

    // Buforowanie odbioru z sieci
    char recv_buffer[sizeof(struct Creature)]; // bufor na odbierane dane
    size_t received_bytes;                     // ile już odebrano
    int receiving;                             // czy aktualnie trwa odbiór
} Creature;

// Inicjalizacja stworzenia
void init_creature(Creature *c);
void update_creature(Creature *c);

// Funkcja ustawiająca tymczasowy ASCII art na określony czas
void set_temp_ascii_art(Creature *c, char *new_art, Uint32 duration_ms);

// Odbieranie stworzonka fragmentami — non-blocking
int try_receive_creature(int sock, Creature *c); // Zwraca 1 jeśli zakończono odbiór, 0 jeśli w trakcie, -1 przy błędzie

#endif // CREATURE_H
