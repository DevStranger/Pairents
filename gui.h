#ifndef GUI_H
#define GUI_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "creature.h"

#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600

#define BUTTON_COUNT 5
#define BUTTON_WIDTH  100
#define BUTTON_HEIGHT 40

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Rect buttons[BUTTON_COUNT];
    TTF_Font *font_text;      // font na teksty przy paskach, przyciskach itd.
    TTF_Font *font_ascii_art; // font do ASCII arta (Matrixtype)
} GUI;

// Inicjalizacja SDL, TTF, okna i renderera oraz pozycji przycisków
int gui_init(GUI *gui);

// Zwolnienie zasobów SDL i TTF
void gui_destroy(GUI *gui);

// Rysowanie pasków postępu (hunger, happiness itd.) dla stworzenia
void gui_draw_creature_status(GUI *gui, Creature *creature, TTF_Font *font_text, TTF_Font *font_emoji);

// Rysowanie całego GUI: tła, pasków i przycisków
void gui_draw_buttons(GUI *gui, Creature *creature, TTF_Font *font_text, TTF_Font *font_emoji);

// Sprawdzenie, czy kliknięcie (x,y) wpadło na któryś z przycisków
// Zwraca indeks przycisku (0..BUTTON_COUNT-1) lub -1, jeśli poza przyciskami
int gui_check_button_click(GUI *gui, int x, int y);

// Funkcje pomocnicze do rysowania pasków i tekstu - nie są eksportowane na zewnątrz (mogą być statyczne w gui.c)
// Nie deklarujemy ich tutaj, bo to funkcje prywatne

#endif // GUI_H
