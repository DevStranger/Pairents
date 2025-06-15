#ifndef GUI_H
#define GUI_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define BUTTON_COUNT 5

// Inicjalizacja SDL i czcionek
int GUI_Init(void);

// Tworzy okno z tytułem
int GUI_CreateWindow(const char *title);

// Rysuje przyciski
void GUI_DrawButtons(SDL_Rect buttons[]);

// Inicjalizuje pozycje przycisków
void GUI_InitButtons(SDL_Rect buttons[]);

// Obsługuje kliknięcia, zwraca indeks klikniętego przycisku lub -1
int GUI_HandleClick(int x, int y, SDL_Rect buttons[]);

// Zamyka okno i renderer
void GUI_Destroy(void);

// Czyści SDL i TTF
void GUI_Quit(void);

#endif // GUI_H
