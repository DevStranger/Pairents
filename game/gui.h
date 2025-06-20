#ifndef GUI_H
#define GUI_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "creature.h"

// wymiary apki
#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 520

// guziki
#define BUTTON_COUNT 5
#define BUTTON_WIDTH  100
#define BUTTON_HEIGHT 40

// zasoby gui
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Rect buttons[BUTTON_COUNT];
    TTF_Font *font_text;    
    TTF_Font *font_ascii_art_default;
    TTF_Font *font_ascii_art_small;
} GUI;

// funkcje gui
int gui_init(GUI *gui);
void gui_destroy(GUI *gui);
void gui_draw_creature_status(GUI *gui, Creature *creature, TTF_Font *font_text, TTF_Font *font_emoji);
void gui_draw_buttons(GUI *gui, Creature *creature, const char *ascii_art, int is_default_ascii_art, TTF_Font *font_text, TTF_Font *font_emoji);
int gui_check_button_click(GUI *gui, int x, int y);
void gui_draw_message(GUI *gui, const char *message, TTF_Font *font_text);

#endif // GUI_H
