#include "gui.h"
#include "creature.h"
#include <stdio.h>
#include <string.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>
#include <stdbool.h>

const char *button_labels[BUTTON_COUNT] = { "Feed", "Read", "Sleep", "Hug", "Play" };

// Wczytaj cały plik tekstowy do dynamicznego bufora (zwraca NULL w razie błędu)
char *load_ascii_art(char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Nie można otworzyć pliku %s\n", filename);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buffer = malloc(size + 1);
    if (!buffer) {
        fclose(f);
        fprintf(stderr, "Brak pamięci na wczytanie ASCII art\n");
        return NULL;
    }
    fread(buffer, 1, size, f);
    buffer[size] = '\0';
    fclose(f);
    return buffer;
}

// GLOBALNA zmienna na ASCII art zajęcia — modyfikowalna, bez const
static char *bunny_ascii = NULL;

static void draw_bar(SDL_Renderer *r, int x, int y, int w, int h, int value, SDL_Color color) {
    SDL_Rect bg = {x, y, w, h};
    SDL_SetRenderDrawColor(r, 50, 50, 50, 255);
    SDL_RenderFillRect(r, &bg);

    SDL_Rect fg = {x, y, w * value / 100, h};
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, 255);
    SDL_RenderFillRect(r, &fg);
}

static void draw_text(SDL_Renderer *r, TTF_Font *font, char *text, int x, int y, SDL_Color color) {
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

static void draw_ascii_art(SDL_Renderer *r, TTF_Font *font, char *ascii_art, int x, int y, SDL_Color color) {
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

    int margin = 20;
    int spacing = 10;
    int available_width = WINDOW_WIDTH - 2 * margin;
    int button_width = (available_width - spacing * (BUTTON_COUNT - 1)) / BUTTON_COUNT;
    int total_buttons_width = button_width * BUTTON_COUNT + spacing * (BUTTON_COUNT - 1);
    int start_x = (WINDOW_WIDTH - total_buttons_width) / 2;

    for (int i = 0; i < BUTTON_COUNT; ++i) {
        gui->buttons[i].x = start_x + i * (button_width + spacing);
        gui->buttons[i].y = WINDOW_HEIGHT - BUTTON_HEIGHT - margin;
        gui->buttons[i].w = button_width;
        gui->buttons[i].h = BUTTON_HEIGHT;
    }

    // Wczytaj bunny_ascii z pliku przy inicjalizacji (raz)
    bunny_ascii = load_ascii_art("bunny.txt");
    if (!bunny_ascii) {
        fprintf(stderr, "Nie udało się wczytać ASCII art zajęcia!\n");
        // możesz tu chcieć zwolnić zasoby i wyjść
        return -1;
    }

    return 0;
}

void gui_destroy(GUI *gui) {
    if (bunny_ascii) {
        free(bunny_ascii);
        bunny_ascii = NULL;
    }
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

    // Read
    draw_text(gui->renderer, font_emoji, "📚", base_x, base_y + 3 * line_height, white);
    draw_text(gui->renderer, font_text, "Read", base_x + 40, base_y + 3 * line_height, white);
    draw_bar(gui->renderer, base_x + 170, base_y + 3 * line_height + 4, 200, 20, creature->read, pink);
    sprintf(buf, "%d%%", creature->read);
    draw_text(gui->renderer, font_text, buf, base_x + 385, base_y + 3 * line_height, white);

    // Hug
    draw_text(gui->renderer, font_emoji, "🤗", base_x, base_y + 4 * line_height, white);
    draw_text(gui->renderer, font_text, "Hug", base_x + 40, base_y + 4 * line_height, white);
    draw_bar(gui->renderer, base_x + 170, base_y + 4 * line_height + 4, 200, 20, creature->hug, orange);
    sprintf(buf, "%d%%", creature->hug);
    draw_text(gui->renderer, font_text, buf, base_x + 385, base_y + 4 * line_height, white);
}

void gui_draw_buttons(GUI *gui, TTF_Font *font) {
    SDL_Color button_bg = {70, 70, 70, 255};
    SDL_Color button_fg = {200, 200, 200, 255};
    SDL_Color button_border = {255, 255, 255, 255};

    for (int i = 0; i < BUTTON_COUNT; ++i) {
        SDL_Rect r = { gui->buttons[i].x, gui->buttons[i].y, gui->buttons[i].w, gui->buttons[i].h };
        // Tło
        SDL_SetRenderDrawColor(gui->renderer, button_bg.r, button_bg.g, button_bg.b, button_bg.a);
        SDL_RenderFillRect(gui->renderer, &r);
        // Obramowanie
        SDL_SetRenderDrawColor(gui->renderer, button_border.r, button_border.g, button_border.b, button_border.a);
        SDL_RenderDrawRect(gui->renderer, &r);
        // Tekst
        int text_w, text_h;
        TTF_SizeUTF8(font, button_labels[i], &text_w, &text_h);
        int text_x = gui->buttons[i].x + (gui->buttons[i].w - text_w) / 2;
        int text_y = gui->buttons[i].y + (gui->buttons[i].h - text_h) / 2;
        draw_text(gui->renderer, font, (char *)button_labels[i], text_x, text_y, button_fg);
    }

    // Rysuj ASCII art zajęcia
    if (bunny_ascii) {
        draw_ascii_art(gui->renderer, font, bunny_ascii, 30, 380, button_fg);
    }
}

bool gui_is_button_clicked(GUI *gui, int x, int y, int *out_button_index) {
    for (int i = 0; i < BUTTON_COUNT; ++i) {
        SDL_Rect r = gui->buttons[i];
        if (x >= r.x && x <= r.x + r.w &&
            y >= r.y && y <= r.y + r.h) {
            if (out_button_index) *out_button_index = i;
            return true;
        }
    }
    return false;
}
