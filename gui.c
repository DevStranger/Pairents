#include "gui.h"
#include "creature.h"
#include <stdio.h>
#include <string.h>
#include <SDL2/SDL_ttf.h>

const char *button_labels[BUTTON_COUNT] = { "Feed", "Read", "Sleep", "Hug", "Play" };

static void draw_bar(SDL_Renderer *r, int x, int y, int w, int h, int value, SDL_Color color) {
    SDL_Rect bg = {x, y, w, h};
    SDL_SetRenderDrawColor(r, 50, 50, 50, 255);
    SDL_RenderFillRect(r, &bg);

    SDL_Rect fg = {x, y, w * value / 100, h};
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, 255);
    SDL_RenderFillRect(r, &fg);
}

static void draw_text(SDL_Renderer *r, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text, color);
    if (!surface) {
        fprintf(stderr, "TTF_RenderUTF8_Blended failed: %s\n", TTF_GetError());
        return;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(r, surface);
    if (!texture) {
        fprintf(stderr, "SDL_CreateTextureFromSurface failed: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        return;
    }
    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(r, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

static void draw_ascii_art(SDL_Renderer *r, TTF_Font *font, const char *ascii_art, int x, int y, SDL_Color color) {
    int line_height = TTF_FontHeight(font);
    const char *line_start = ascii_art;
    int line_num = 0;

    while (*line_start) {
        const char *line_end = strchr(line_start, '\n');
        int len = line_end ? (line_end - line_start) : strlen(line_start);

        char buffer[256];
        if (len > 255) len = 255;
        strncpy(buffer, line_start, len);
        buffer[len] = '\0';

        draw_text(r, font, buffer, x, y + line_num * line_height, color);

        if (!line_end) break;
        line_start = line_end + 1;
        line_num++;
    }
}

int gui_init(GUI *gui) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL Init Error: %s\n", SDL_GetError());
        return -1;
    }

    if (TTF_Init() == -1) {
        fprintf(stderr, "TTF_Init Error: %s\n", TTF_GetError());
        SDL_Quit();
        return -1;
    }

    gui->font_text = TTF_OpenFont("assets/SomeRegularFont.ttf", 12);
    if (!gui->font_text) {
        fprintf(stderr, "TTF_OpenFont font_text failed: %s\n", TTF_GetError());
        return -1;
    }

    gui->font_ascii_art = TTF_OpenFont("assets/MatrixtypeDisplayBold-6R4e6.ttf", 6);
    if (!gui->font_ascii_art) {
        fprintf(stderr, "TTF_OpenFont font_ascii_art failed: %s\n", TTF_GetError());
        TTF_CloseFont(gui->font_text);
        return -1;
    }

    gui->window = SDL_CreateWindow("Wybierz akcję",
                                   SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                   WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!gui->window) {
        fprintf(stderr, "SDL CreateWindow Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return -1;
    }

    gui->renderer = SDL_CreateRenderer(gui->window, -1, SDL_RENDERER_ACCELERATED);
    if (!gui->renderer) {
        fprintf(stderr, "SDL CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(gui->window);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }

    // Parametry układu
    int margin = 20;              // margines z lewej i prawej
    int spacing = 10;             // odstęp między guzikami
    int available_width = WINDOW_WIDTH - 2 * margin;

    // Dynamiczne obliczenie szerokości guzika
    int button_width = (available_width - spacing * (BUTTON_COUNT - 1)) / BUTTON_COUNT;

    // Obliczamy start_x, by całość była wyśrodkowana
    int total_buttons_width = button_width * BUTTON_COUNT + spacing * (BUTTON_COUNT - 1);
    int start_x = (WINDOW_WIDTH - total_buttons_width) / 2;

    for (int i = 0; i < BUTTON_COUNT; ++i) {
        gui->buttons[i].x = start_x + i * (button_width + spacing);
        gui->buttons[i].y = WINDOW_HEIGHT - BUTTON_HEIGHT - margin;
        gui->buttons[i].w = button_width;
        gui->buttons[i].h = BUTTON_HEIGHT;
    }

    return 0;
}

void gui_destroy(GUI *gui) {
    if (gui->font_ascii_art) TTF_CloseFont(gui->font_ascii_art);
    if (gui->font_text) TTF_CloseFont(gui->font_text);
    if (gui->renderer) SDL_DestroyRenderer(gui->renderer);
    if (gui->window) SDL_DestroyWindow(gui->window);
    TTF_Quit();
    SDL_Quit();
}

void gui_draw_creature_status(GUI *gui, Creature *creature, TTF_Font *font_text, TTF_Font *font_emoji) {
    SDL_Color white  = {255, 255, 255, 255};
    SDL_Color green  = {0, 200, 0, 255};
    SDL_Color blue   = {0, 100, 255, 255};
    SDL_Color red    = {255, 50, 50, 255};
    SDL_Color yellow = {255, 255, 0, 255};
    SDL_Color pink   = {255, 105, 180, 255};
    SDL_Color orange = {255, 140, 0, 255};

    int base_x = 20;
    int base_y = 20;
    int line_height = 30;

    char buf[16];

    SDL_SetRenderDrawColor(gui->renderer, 30, 30, 30, 200);
    SDL_Rect bg = {base_x - 10, base_y - 10, 450, line_height * 6 + 20};
    SDL_RenderFillRect(gui->renderer, &bg);

    // Hunger
    draw_text(gui->renderer, font_emoji, "🍎", base_x, base_y, white);
    draw_text(gui->renderer, font_text, "Hunger", base_x + 40, base_y, white);
    draw_bar(gui->renderer, base_x + 170, base_y + 4, 200, 20, creature->hunger, green);
    sprintf(buf, "%d%%", creature->hunger);
    draw_text(gui->renderer, font_text, buf, base_x + 385, base_y, white);

    // Happiness
    draw_text(gui->renderer, font_emoji, "🌼", base_x, base_y + line_height, white);
    draw_text(gui->renderer, font_text, "Happiness", base_x + 40, base_y + line_height, white);
    draw_bar(gui->renderer, base_x + 170, base_y + line_height + 4, 200, 20, creature->happiness, yellow);
    sprintf(buf, "%d%%", creature->happiness);
    draw_text(gui->renderer, font_text, buf, base_x + 385, base_y + line_height, white);

    // Sleep
    draw_text(gui->renderer, font_emoji, "💤", base_x, base_y + 2 * line_height, white);
    draw_text(gui->renderer, font_text, "Sleep", base_x + 40, base_y + 2 * line_height, white);
    draw_bar(gui->renderer, base_x + 170, base_y + 2 * line_height + 4, 200, 20, creature->sleep, blue);
    sprintf(buf, "%d%%", creature->sleep);
    draw_text(gui->renderer, font_text, buf, base_x + 385, base_y + 2 * line_height, white);

    // Health
    draw_text(gui->renderer, font_emoji, "💊", base_x, base_y + 3 * line_height, white);
    draw_text(gui->renderer, font_text, "Health", base_x + 40, base_y + 3 * line_height, white);
    draw_bar(gui->renderer, base_x + 170, base_y + 3 * line_height + 4, 200, 20, creature->health, red);
    sprintf(buf, "%d%%", creature->health);
    draw_text(gui->renderer, font_text, buf, base_x + 385, base_y + 3 * line_height, white);

    // Growth
    draw_text(gui->renderer, font_emoji, "🌱", base_x, base_y + 4 * line_height, white);
    draw_text(gui->renderer, font_text, "Growth", base_x + 40, base_y + 4 * line_height, white);
    draw_bar(gui->renderer, base_x + 170, base_y + 4 * line_height + 4, 200, 20, creature->growth, pink);
    sprintf(buf, "%d%%", creature->growth);
    draw_text(gui->renderer, font_text, buf, base_x + 385, base_y + 4 * line_height, white);

    // Love
    draw_text(gui->renderer, font_emoji, "❤️", base_x, base_y + 5 * line_height, white);
    draw_text(gui->renderer, font_text, "Love", base_x + 40, base_y + 5 * line_height, white);
    draw_bar(gui->renderer, base_x + 170, base_y + 5 * line_height + 4, 200, 20, creature->love, orange);
    sprintf(buf, "%d%%", creature->love);
    draw_text(gui->renderer, font_text, buf, base_x + 385, base_y + 5 * line_height, white);
}

void gui_draw_buttons(GUI *gui, Creature *creature, TTF_Font *font_text, TTF_Font *font_emoji) {
    // Czyścimy ekran
    SDL_SetRenderDrawColor(gui->renderer, 50, 50, 100, 255);
    SDL_RenderClear(gui->renderer);

    // Rysujemy wskaźniki stwora (paski życia, sen, miłość, itd.)
    gui_draw_creature_status(gui, creature, font_text, font_emoji);

    // Rysujemy ASCII Arta
    SDL_Color white = {255, 255, 255, 255};
    int art_x = 120;  
    int art_y = 80;
    draw_ascii_art(gui->renderer, gui->font_ascii_art, creature->ascii_art, art_x, art_y, white);

    // Rysujemy guziki
    for (int i = 0; i < BUTTON_COUNT; ++i) {
        SDL_SetRenderDrawColor(gui->renderer, 100, 100, 255, 255);
        SDL_RenderFillRect(gui->renderer, &gui->buttons[i]);

        // Rysujemy etykietę
        draw_text(gui->renderer, font_text, button_labels[i],
                  gui->buttons[i].x + 10, gui->buttons[i].y + 10, (SDL_Color){255,255,255,255});
    }

    SDL_RenderPresent(gui->renderer);
}

int gui_check_button_click(GUI *gui, int x, int y) {
    for (int i = 0; i < BUTTON_COUNT; ++i) {
        SDL_Rect *btn = &gui->buttons[i];
        if (x >= btn->x && x <= btn->x + btn->w &&
            y >= btn->y && y <= btn->y + btn->h) {
            return i;
        }
    }
    return -1;
}
