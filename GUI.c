#include "gui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SDL_Rect buttons[BUTTON_COUNT];

const char *button_symbols[BUTTON_COUNT] = { "🍓", "📖", "💤", "🤗", "🎡" };
const char *button_labels[BUTTON_COUNT]  = { "Feed", "Read", "Sleep", "Hug", "Play" };

static char *bunny_art = NULL;
static TTF_Font *font_text  = NULL;
static TTF_Font *font_emoji = NULL;
static TTF_Font *font_bunny = NULL;

// Do animacji przycisku klikniętego
static int last_clicked_button = -1;
static Uint32 last_click_time = 0;

// Ładuje ASCII art z pliku
static char* load_ascii_art_from_file(const char *filepath) {
    FILE *f = fopen(filepath, "r");
    if (!f) {
        perror("load_ascii_art_from_file");
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buffer = malloc(size + 1);
    if (!buffer) {
        fclose(f);
        return NULL;
    }
    fread(buffer, 1, size, f);
    buffer[size] = '\0';
    fclose(f);
    return buffer;
}

static void draw_text(SDL_Renderer *r, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text, color);
    if (!surface) return;
    SDL_Texture *texture = SDL_CreateTextureFromSurface(r, surface);
    if (!texture) {
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

static void draw_bar(SDL_Renderer *r, int x, int y, int w, int h, int value, SDL_Color color) {
    SDL_Rect bg = {x, y, w, h};
    SDL_SetRenderDrawColor(r, 50, 50, 50, 255);
    SDL_RenderFillRect(r, &bg);

    SDL_Rect fg = {x, y, w * value / 100, h};
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, 255);
    SDL_RenderFillRect(r, &fg);
}

void draw_buttons(SDL_Renderer *renderer, TTF_Font *font_regular, TTF_Font *font_emoji, SDL_Color color) {
    Uint32 now = SDL_GetTicks();

    for (int i = 0; i < BUTTON_COUNT; i++) {
        buttons[i].x = 20 + i * 160;
        buttons[i].y = 420;
        buttons[i].w = 140;
        buttons[i].h = 40;

        if (i == last_clicked_button && now - last_click_time < 300) {
            SDL_SetRenderDrawColor(renderer, 70, 70, 120, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        }
        SDL_RenderFillRect(renderer, &buttons[i]);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &buttons[i]);

        int x = buttons[i].x + 10;
        int y = buttons[i].y + 10;

        draw_text(renderer, font_emoji, button_symbols[i], x, y, color);

        int w_symbol = 0, h_symbol = 0;
        TTF_SizeText(font_emoji, button_symbols[i], &w_symbol, &h_symbol);

        draw_text(renderer, font_regular, button_labels[i], x + w_symbol + 5, y, color);
    }
}

int gui_init(SDL_Renderer *renderer) {
    font_text  = TTF_OpenFont("client/assets/fonts/MatrixtypeDisplayBold-6R4e6.ttf", 14);
    font_emoji = TTF_OpenFont("client/assets/fonts/NotoEmoji-VariableFont_wght.ttf", 18);
    font_bunny = TTF_OpenFont("client/assets/fonts/MatrixtypeDisplayBold-6R4e6.ttf", 6);
    if (!font_text || !font_emoji || !font_bunny) {
        fprintf(stderr, "Brak czcionki! %s\n", TTF_GetError());
        return 0;
    }
    bunny_art = load_ascii_art_from_file("client/assets/bunny/default.txt");
    if (!bunny_art) {
        fprintf(stderr, "Nie można załadować ASCII art\n");
        return 0;
    }
    return 1;
}

void gui_cleanup(void) {
    free(bunny_art);
    bunny_art = NULL;

    if (font_text) {
        TTF_CloseFont(font_text);
        font_text = NULL;
    }
    if (font_emoji) {
        TTF_CloseFont(font_emoji);
        font_emoji = NULL;
    }
    if (font_bunny) {
        TTF_CloseFont(font_bunny);
        font_bunny = NULL;
    }
}

// Rysowanie GUI - tutaj można rozszerzyć o paski itp.
void gui_render(SDL_Renderer *renderer) {
    SDL_Color white  = {255, 255, 255, 255};
    SDL_Color green  = {0, 200, 0, 255};
    SDL_Color blue   = {0, 100, 255, 255};
    SDL_Color red    = {255, 50, 50, 255};
    SDL_Color yellow = {255, 255, 0, 255};
    SDL_Color pink   = {255, 105, 180, 255};
    SDL_Color orange = {255, 140, 0, 255};

    // Czyszczenie ekranu
    SDL_SetRenderDrawColor(renderer, 100, 149, 237, 255);
    SDL_RenderClear(renderer);

    // Tutaj jako przykład - rysowanie kilku pasków ze statystykami
    // (przekaż wartości w implementacji klienta)
    int hunger = 75;
    int happiness = 60;
    int sleep = 45;
    int health = 90;
    int growth = 30;
    int love = 80;

    draw_text(renderer, font_emoji, "🍎", 20,  40, white);
    draw_text(renderer, font_text, "Hunger", 60, 40, white);
    draw_bar(renderer, 155, 42, 200, 16, hunger, green);
    char hunger_value[8];
    sprintf(hunger_value, "%d%%", hunger);
    draw_text(renderer, font_text, hunger_value, 360, 40, white);

    draw_text(renderer, font_emoji, "🌼", 20,  70, white);
    draw_text(renderer, font_text, "Happiness", 60, 70, white);
    draw_bar(renderer, 155, 72, 200, 16, happiness, yellow);
    char happiness_value[8];
    sprintf(happiness_value, "%d%%", happiness);
    draw_text(renderer, font_text, happiness_value, 360, 70, white);

    draw_text(renderer, font_emoji, "💤", 20, 100, white);
    draw_text(renderer, font_text, "Sleep", 60, 100, white);
    draw_bar(renderer, 155, 102, 200, 16, sleep, blue);
    char sleep_value[8];
    sprintf(sleep_value, "%d%%", sleep);
    draw_text(renderer, font_text, sleep_value, 360, 100, white);

    draw_text(renderer, font_emoji, "💊", 20, 130, white);
    draw_text(renderer, font_text, "Health", 60, 130, white);
    draw_bar(renderer, 155, 132, 200, 16, health, red);
    char health_value[8];
    sprintf(health_value, "%d%%", health);
    draw_text(renderer, font_text, health_value, 360, 130, white);

    draw_text(renderer, font_emoji, "🌱", 20, 160, white);
    draw_text(renderer, font_text, "Growth", 60, 160, white);
    draw_bar(renderer, 155, 162, 200, 16, growth, pink);
    char growth_value[8];
    sprintf(growth_value, "%d%%", growth);
    draw_text(renderer, font_text, growth_value, 360, 160, white);

    draw_text(renderer, font_emoji, "❤️", 20, 190, white);
    draw_text(renderer, font_text, "Love", 60, 190, white);
    draw_bar(renderer, 155, 192, 200, 16, love, orange);
    char love_value[8];
    sprintf(love_value, "%d%%", love);
    draw_text(renderer, font_text, love_value, 360, 190, white);

    // Rysowanie ASCII art
    if (bunny_art) {
        draw_ascii_art(renderer, font_bunny, bunny_art, 450, 50, white);
    }

    // Rysowanie przycisków
    draw_buttons(renderer, font_text, font_emoji, white);
}

int gui_check_button_click(int x, int y) {
    Uint32 now = SDL_GetTicks();
    for (int i = 0; i < BUTTON_COUNT; i++) {
        if (x >= buttons[i].x && x <= buttons[i].x + buttons[i].w &&
            y >= buttons[i].y && y <= buttons[i].y + buttons[i].h) {
            last_clicked_button = i;
            last_click_time = now;
            return i;
        }
    }
    return -1;
}
