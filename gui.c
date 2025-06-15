#include "gui.h"
#include <stdio.h>

const char *button_labels[BUTTON_COUNT] = { "Feed", "Read", "Sleep", "Hug", "Play" };

int gui_init(GUI *gui) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL Init Error: %s\n", SDL_GetError());
        return -1;
    }

    gui->window = SDL_CreateWindow("Wybierz akcję",
                                   SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                   WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!gui->window) {
        fprintf(stderr, "SDL CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    gui->renderer = SDL_CreateRenderer(gui->window, -1, SDL_RENDERER_ACCELERATED);
    if (!gui->renderer) {
        fprintf(stderr, "SDL CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(gui->window);
        SDL_Quit();
        return -1;
    }

    for (int i = 0; i < BUTTON_COUNT; ++i) {
        gui->buttons[i].x = 20 + i * (BUTTON_WIDTH + 10);
        gui->buttons[i].y = 50;
        gui->buttons[i].w = BUTTON_WIDTH;
        gui->buttons[i].h = BUTTON_HEIGHT;
    }

    return 0;
}

void gui_destroy(GUI *gui) {
    if (gui->renderer) SDL_DestroyRenderer(gui->renderer);
    if (gui->window) SDL_DestroyWindow(gui->window);
    SDL_Quit();
}

void gui_draw_buttons(GUI *gui) {
    SDL_SetRenderDrawColor(gui->renderer, 200, 200, 200, 255);
    SDL_RenderClear(gui->renderer);

    for (int i = 0; i < BUTTON_COUNT; ++i) {
        SDL_SetRenderDrawColor(gui->renderer, 100, 100, 255, 255);
        SDL_RenderFillRect(gui->renderer, &gui->buttons[i]);
    }

    SDL_RenderPresent(gui->renderer);
}

int gui_check_button_click(GUI *gui, int x, int y) {
    for (int i = 0; i < BUTTON_COUNT; ++i) {
        SDL_Rect *btn = &gui->buttons[i];
        if (x >= btn->x && x <= btn->x + btn->w &&
            y >= btn->y && y <= btn->y + btn->h) {
            return i;  // zwraca indeks przycisku
        }
    }
    return -1; // brak kliknięcia na przycisk
}
