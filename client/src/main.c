#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "creature.h"

#define WINDOW_WIDTH 840
#define WINDOW_HEIGHT 480

// deklaracja globalna wskaźnika do ASCII
char *bunny_art = NULL;

// --- NOWOŚĆ: przyciski akcji ---
#define BUTTON_COUNT 5
SDL_Rect buttons[BUTTON_COUNT];
const char *button_labels[BUTTON_COUNT] = {"🍓 Feed", "📖 Read", "💤 Sleep", "🤗 Hug", "🎡 Play"};

void draw_bar(SDL_Renderer *r, int x, int y, int w, int h, int value, SDL_Color color) {
    SDL_Rect outer = { x, y, w, h };
    SDL_Rect inner = { x + 2, y + 2, (w - 4) * value / 100, h - 4 };

    SDL_SetRenderDrawColor(r, 80, 80, 80, 255); 
    SDL_RenderFillRect(r, &outer);

    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, 255);
    SDL_RenderFillRect(r, &inner);
}

void draw_text(SDL_Renderer *r, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(r, surface);
    SDL_Rect dst = { x, y, surface->w, surface->h };
    SDL_RenderCopy(r, texture, NULL, &dst);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void draw_ascii_art(SDL_Renderer *r, TTF_Font *font, const char *ascii_art, int x, int y, SDL_Color color) {
    const char *line = ascii_art;
    int offset_y = 0;

    while (line && *line) {
        const char *next_line = strchr(line, '\n');
        int len = next_line ? (int)(next_line - line) : strlen(line);

        char temp[1024];
        strncpy(temp, line, len);
        temp[len] = '\0';

        SDL_Surface *surface = TTF_RenderText_Solid(font, temp, color);
        if (surface) {
            SDL_Texture *texture = SDL_CreateTextureFromSurface(r, surface);
            SDL_Rect dst = {x, y + offset_y, surface->w, surface->h};
            SDL_RenderCopy(r, texture, NULL, &dst);
            offset_y += surface->h; 
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        }

        line = next_line ? next_line + 1 : NULL;
    }
}

char* load_ascii_art_from_file(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        printf("Nie mogę otworzyć pliku: %s\n", filepath);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char *buffer = malloc(size + 1);
    if (!buffer) {
        fclose(file);
        printf("Błąd alokacji pamięci.\n");
        return NULL;
    }

    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    fclose(file);
    return buffer;
}

void draw_buttons(SDL_Renderer *renderer, TTF_Font *font, SDL_Color color) {
    for (int i = 0; i < BUTTON_COUNT; i++) {
        buttons[i].x = 20 + i * 160;
        buttons[i].y = 420;
        buttons[i].w = 140;
        buttons[i].h = 40;

        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderFillRect(renderer, &buttons[i]);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &buttons[i]);
        draw_text(renderer, font, button_labels[i], buttons[i].x + 10, buttons[i].y + 10, color);
    }
}

int check_button_click(int x, int y) {
    for (int i = 0; i < BUTTON_COUNT; i++) {
        if (x >= buttons[i].x && x <= buttons[i].x + buttons[i].w &&
            y >= buttons[i].y && y <= buttons[i].y + buttons[i].h) {
            return i;
        }
    }
    return -1;
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Window *window = SDL_CreateWindow("Pairents",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    TTF_Font *font_text  = TTF_OpenFont("client/assets/fonts/MatrixtypeDisplayBold-6R4e6.ttf", 14);
    TTF_Font *font_emoji = TTF_OpenFont("client/assets/fonts/NotoEmoji-VariableFont_wght.ttf", 18);
    TTF_Font *font_bunny = TTF_OpenFont("client/assets/fonts/MatrixtypeDisplayBold-6R4e6.ttf", 6); 

    if (!font_text || !font_emoji || !font_bunny) {
        printf("Brak czcionki! %s\n", TTF_GetError());
        return 1;
    }

    bunny_art = load_ascii_art_from_file("client/assets/bunny/default.txt");
    if (!bunny_art) {
        return 1;
    }

    Creature creature;
    init_creature(&creature);
    Uint32 last_update = SDL_GetTicks();

    SDL_Color white  = {255, 255, 255};
    SDL_Color green  = {0, 200, 0};
    SDL_Color blue   = {0, 100, 255};
    SDL_Color red    = {255, 50, 50};
    SDL_Color yellow = {255, 255, 0};
    SDL_Color pink   = {255, 105, 180};
    SDL_Color orange = {255, 140, 0};

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int clicked = check_button_click(event.button.x, event.button.y);
                if (clicked != -1) {
                    printf("Kliknięto: %s\n", button_labels[clicked]);
                    // TODO: send_msg(...) do serwera z akcją
                }
            }
        }

        Uint32 now = SDL_GetTicks();
        if (now - last_update > 1000) {
            update_creature(&creature);
            last_update = now;
        }

        SDL_SetRenderDrawColor(renderer, 100, 149, 237, 255); 
        SDL_RenderClear(renderer);

        draw_text(renderer, font_emoji, "🍎", 20,  40, white);
        draw_text(renderer, font_text, "Hunger", 60, 40, white);
        draw_bar(renderer, 155, 42, 200, 16, creature.hunger, green);
        char hunger_value[8];
        sprintf(hunger_value, "%d%%", creature.hunger);
        draw_text(renderer, font_text, hunger_value, 360, 40, white);

        draw_text(renderer, font_emoji, "🌼", 20,  70, white);
        draw_text(renderer, font_text, "Happiness", 60, 70, white);
        draw_bar(renderer, 155, 72, 200, 16, creature.happiness, yellow);
        char happiness_value[8];
        sprintf(happiness_value, "%d%%", creature.happiness);
        draw_text(renderer, font_text, happiness_value, 360, 70, white);

        draw_text(renderer, font_emoji, "💤", 20, 100, white);
        draw_text(renderer, font_text, "Sleep", 60, 100, white);
        draw_bar(renderer, 155, 102, 200, 16, creature.sleep, blue);
        char sleep_value[8];
        sprintf(sleep_value, "%d%%", creature.sleep);
        draw_text(renderer, font_text, sleep_value, 360, 100, white);

        draw_text(renderer, font_emoji, "💊", 20, 130, white);
        draw_text(renderer, font_text, "Health", 60, 130, white);
        draw_bar(renderer, 155, 132, 200, 16, creature.health, red);
        char health_value[8];
        sprintf(health_value, "%d%%", creature.health);
        draw_text(renderer, font_text, health_value, 360, 130, white);

        draw_text(renderer, font_emoji, "🌱", 20, 160, white);
        draw_text(renderer, font_text, "Growth", 60, 160, white);
        draw_bar(renderer, 155, 162, 200, 16, creature.growth, orange);
        char growth_value[8];
        sprintf(growth_value, "%d%%", creature.growth);
        draw_text(renderer, font_text, growth_value, 360, 160, white);

        draw_text(renderer, font_emoji, "❤️", 20, 190, white);
        draw_text(renderer, font_text, "Love", 60, 190, white);
        draw_bar(renderer, 155, 192, 200, 16, creature.love, pink);
        char love_value[8];
        sprintf(love_value, "%d%%", creature.love);
        draw_text(renderer, font_text, love_value, 360, 190, white);

        draw_ascii_art(renderer, font_bunny, bunny_art, 420, 30, white); 

        draw_buttons(renderer, font_text, white);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    free(bunny_art);
    TTF_CloseFont(font_text);
    TTF_CloseFont(font_emoji);
    TTF_CloseFont(font_bunny);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
