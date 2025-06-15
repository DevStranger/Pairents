#ifndef GUI_H
#define GUI_H

#include <SDL2/SDL.h>

#define BUTTON_COUNT 5
#define BUTTON_WIDTH 90
#define BUTTON_HEIGHT 60
#define WINDOW_WIDTH 500
#define WINDOW_HEIGHT 200

extern const char *button_labels[BUTTON_COUNT];

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Rect buttons[BUTTON_COUNT];
} GUI;

int gui_init(GUI *gui);
void gui_destroy(GUI *gui);
void gui_draw_buttons(GUI *gui);
int gui_check_button_click(GUI *gui, int x, int y);

#endif // GUI_H
