#include "gui.h"
#include "creature.h"
#include <stdio.h>
#include <string.h>
#include <SDL2/SDL_ttf.h>

// etykiety guzików
const char *button_labels[BUTTON_COUNT] = { "Feed", "Read", "Sleep", "Hug", "Play" };

// funkcja rysująca paski postepu na wskaźniki (+ tło dla nich)
static void draw_bar(SDL_Renderer *r, int x, int y, int w, int h, int value, SDL_Color color) {
    SDL_Rect bg = {x, y, w, h};
    SDL_SetRenderDrawColor(r, 50, 50, 50, 255);
    SDL_RenderFillRect(r, &bg);

    SDL_Rect fg = {x, y, w * value / 100, h};
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, 255);
    SDL_RenderFillRect(r, &fg);
}

// funkcja rysująca tekst
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

// funkcja rysująca ascii art stwora
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

// funkcja do inicjalizacji gui
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

    gui->font_text = TTF_OpenFont("../assets/MatrixtypeDisplayBold-6R4e6.ttf", 14);
    if (!gui->font_text) {
        fprintf(stderr, "TTF_OpenFont font_text failed: %s\n", TTF_GetError());
        return -1;
    }

    gui->font_ascii_art_default = TTF_OpenFont("../assets/MatrixtypeDisplayBold-6R4e6.ttf", 6);
    gui->font_ascii_art_small = TTF_OpenFont("../assets/MatrixtypeDisplayBold-6R4e6.ttf", 4);

    gui->window = SDL_CreateWindow("Pairents",
                                   SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                   WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!gui->window) {
        fprintf(stderr, "SDL CreateWindow Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return -1;
    }

    // renderer z akceleracją i synchronizacją pionową 
    gui->renderer = SDL_CreateRenderer(gui->window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!gui->renderer) {
        fprintf(stderr, "SDL CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(gui->window);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }

    // obliczanie rozmiarów i położeń przycisków z marginesami i odstępami
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

    return 0;
}

// funkcja zwalniająca zasoby (fonty, renderer, okna i wyłączenie SDL/TTF)
void gui_destroy(GUI *gui) {
    if (gui->font_ascii_art_default) {
        TTF_CloseFont(gui->font_ascii_art_default);
        gui->font_ascii_art_default = NULL;
    }
    if (gui->font_ascii_art_small) {
        TTF_CloseFont(gui->font_ascii_art_small);
        gui->font_ascii_art_small = NULL;
    }
    if (gui->font_text) {
        TTF_CloseFont(gui->font_text);
        gui->font_text = NULL;
    }
    if (gui->renderer) {
        SDL_DestroyRenderer(gui->renderer);
        gui->renderer = NULL;
    }
    if (gui->window) {
        SDL_DestroyWindow(gui->window);
        gui->window = NULL;
    }
    TTF_Quit();
    SDL_Quit();
}

// funkcja rysująca wskaźniki stwora
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

    // Hunger (głód)
    draw_text(gui->renderer, font_emoji, "🍎", base_x, base_y, white);
    draw_text(gui->renderer, font_text, "Hunger", base_x + 40, base_y, white);
    draw_bar(gui->renderer, base_x + 170, base_y + 4, 200, 20, creature->hunger, green);
    sprintf(buf, "%d%%", creature->hunger);
    draw_text(gui->renderer, font_text, buf, base_x + 385, base_y, white);

    // Happiness (szczęście)
    draw_text(gui->renderer, font_emoji, "🌼", base_x, base_y + line_height, white);
    draw_text(gui->renderer, font_text, "Happiness", base_x + 40, base_y + line_height, white);
    draw_bar(gui->renderer, base_x + 170, base_y + line_height + 4, 200, 20, creature->happiness, yellow);
    sprintf(buf, "%d%%", creature->happiness);
    draw_text(gui->renderer, font_text, buf, base_x + 385, base_y + line_height, white);

    // Sleep (sen)
    draw_text(gui->renderer, font_emoji, "💤", base_x, base_y + 2 * line_height, white);
    draw_text(gui->renderer, font_text, "Sleep", base_x + 40, base_y + 2 * line_height, white);
    draw_bar(gui->renderer, base_x + 170, base_y + 2 * line_height + 4, 200, 20, creature->sleep, blue);
    sprintf(buf, "%d%%", creature->sleep);
    draw_text(gui->renderer, font_text, buf, base_x + 385, base_y + 2 * line_height, white);

    // Health (zdrowie)
    draw_text(gui->renderer, font_emoji, "💊", base_x, base_y + 3 * line_height, white);
    draw_text(gui->renderer, font_text, "Health", base_x + 40, base_y + 3 * line_height, white);
    draw_bar(gui->renderer, base_x + 170, base_y + 3 * line_height + 4, 200, 20, creature->health, red);
    sprintf(buf, "%d%%", creature->health);
    draw_text(gui->renderer, font_text, buf, base_x + 385, base_y + 3 * line_height, white);

    // Growth (rozwój)
    draw_text(gui->renderer, font_emoji, "🌱", base_x, base_y + 4 * line_height, white);
    draw_text(gui->renderer, font_text, "Growth", base_x + 40, base_y + 4 * line_height, white);
    draw_bar(gui->renderer, base_x + 170, base_y + 4 * line_height + 4, 200, 20, creature->growth, pink);
    sprintf(buf, "%d%%", creature->growth);
    draw_text(gui->renderer, font_text, buf, base_x + 385, base_y + 4 * line_height, white);

    // Love (miłość)
    draw_text(gui->renderer, font_emoji, "❤️", base_x, base_y + 5 * line_height, white);
    draw_text(gui->renderer, font_text, "Love", base_x + 40, base_y + 5 * line_height, white);
    draw_bar(gui->renderer, base_x + 170, base_y + 5 * line_height + 4, 200, 20, creature->love, orange);
    sprintf(buf, "%d%%", creature->love);
    draw_text(gui->renderer, font_text, buf, base_x + 385, base_y + 5 * line_height, white);
}

// funkcja do rysowania poziomu stwora
void gui_draw_level(GUI *gui, int level, TTF_Font *font_text) {
    SDL_Color button_blue = {100, 100, 255, 255};  // kolor jak guziki
    SDL_Color white = {255, 255, 255, 255};        // biały tekst

    int radius = 20;
    int center_x = WINDOW_WIDTH - radius - 10; // 10px od prawego brzegu
    int center_y = radius + 10;                // 10px od góry

    SDL_Rect rect = {center_x - radius, center_y - radius, radius * 2, radius * 2};
    SDL_SetRenderDrawColor(gui->renderer, button_blue.r, button_blue.g, button_blue.b, button_blue.a);
    SDL_RenderFillRect(gui->renderer, &rect);

    // wyświetlenie poziomu na środku
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", level);

    SDL_Surface *surface = TTF_RenderUTF8_Blended(font_text, buf, white);
    if (surface) {
        SDL_Texture *texture = SDL_CreateTextureFromSurface(gui->renderer, surface);
        if (texture) {
            SDL_Rect dst_rect;
            dst_rect.w = surface->w;
            dst_rect.h = surface->h;
            dst_rect.x = center_x - dst_rect.w / 2;
            dst_rect.y = center_y - dst_rect.h / 2;

            SDL_RenderCopy(gui->renderer, texture, NULL, &dst_rect);
            SDL_DestroyTexture(texture);
        }
        SDL_FreeSurface(surface);
    }
}

// funkcja rysująca guziki i całe gui
void gui_draw_buttons(GUI *gui, Creature *creature, const char *ascii_art, int is_default_ascii_art, TTF_Font *font_text, TTF_Font *font_emoji) {
    // czyszczenie ekranu
    SDL_SetRenderDrawColor(gui->renderer, 50, 50, 100, 255);
    SDL_RenderClear(gui->renderer);

    // wskaźniki
    gui_draw_creature_status(gui, creature, font_text, font_emoji);

    // ascii art
    SDL_Color white = {255, 255, 255, 255};
    int art_x = 475;  
    int art_y = 90;

    if (ascii_art) {
        if (is_default_ascii_art) {
            draw_ascii_art(gui->renderer, gui->font_ascii_art_default, ascii_art, art_x, art_y, white);
        } else {
            draw_ascii_art(gui->renderer, gui->font_ascii_art_small, ascii_art, art_x, art_y, white);
        }
    }

    // poziom
    gui_draw_level(gui, creature->level, font_text);
    
    // guziki
    for (int i = 0; i < BUTTON_COUNT; ++i) {
        SDL_SetRenderDrawColor(gui->renderer, 100, 100, 255, 255);
        SDL_RenderFillRect(gui->renderer, &gui->buttons[i]);

        // etykierty guzików
        draw_text(gui->renderer, font_text, button_labels[i],
                  gui->buttons[i].x + 10, gui->buttons[i].y + 10, (SDL_Color){255,255,255,255});
    }
}

// funkcja do sprawdzania kliknięć guzików
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

// funkcja rysująca wiadomości
void gui_draw_message(GUI *gui, const char *message, TTF_Font *font_text) {
    if (!message || strlen(message) == 0) return;

    SDL_Color text_color = {255, 255, 255, 255};
    SDL_Color bg_color = {100, 100, 255, 255}; // jasnoniebieskie tło, takie jak guziki

    int x = 20;
    int y = 230;

    // renderujemy tekst na surface, by poznać jego rozmiar
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font_text, message, text_color);
    if (!surface) {
        fprintf(stderr, "TTF_RenderUTF8_Blended failed: %s\n", TTF_GetError());
        return;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(gui->renderer, surface);
    if (!texture) {
        fprintf(stderr, "SDL_CreateTextureFromSurface failed: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        return;
    }

    // tło tekstu wiadomości
    SDL_Rect bg_rect = { x - 5, y - 5, surface->w + 10, surface->h + 10 };
    SDL_SetRenderDrawColor(gui->renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
    SDL_RenderFillRect(gui->renderer, &bg_rect);

    // tekst wiadomości
    SDL_Rect dst_rect = { x, y, surface->w, surface->h };
    SDL_RenderCopy(gui->renderer, texture, NULL, &dst_rect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}
