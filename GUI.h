#ifndef GUI_H
#define GUI_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "creature.h"

#define BUTTON_COUNT 5

// Inicjalizacja zasobów GUI (czcionki, ASCII art) na podstawie podanego renderer'a SDL
// Zwraca 1 w przypadku sukcesu, 0 w przypadku błędu
int GUI_init(SDL_Renderer *renderer);

// Zwalnia zasoby używane przez GUI (czcionki, ASCII art)
void GUI_cleanup(void);

// Renderuje GUI: statystyki stworzenia, ASCII art oraz przyciski
void GUI_render(SDL_Renderer *renderer, const creature_t *creature);

// Sprawdza, czy kliknięcie (x, y) mieści się w którymś z przycisków
// Zwraca indeks przycisku lub -1, jeśli poza przyciskami
int GUI_check_button_click(int x, int y);

#endif // GUI_H

