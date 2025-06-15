#ifndef GUI_H
#define GUI_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define BUTTON_COUNT 5

// Przechowuje pozycje przycisków (do obsługi kliknięć)
extern SDL_Rect buttons[BUTTON_COUNT];

// Symbole emoji i napisy przycisków
extern const char *button_symbols[BUTTON_COUNT];
extern const char *button_labels[BUTTON_COUNT];

// Inicjalizacja GUI (ładowanie czcionek, ASCII art)
int gui_init(SDL_Renderer *renderer);

// Sprzątanie zasobów GUI
void gui_cleanup(void);

// Rysowanie całego GUI (pasek, ASCII art, przyciski)
void gui_render(SDL_Renderer *renderer);

// Sprawdza, czy kliknięto któryś przycisk, zwraca indeks lub -1
int gui_check_button_click(int x, int y);

#endif // GUI_H
