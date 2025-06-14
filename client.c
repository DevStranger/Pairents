#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 12345
#define SERVER_IP "127.0.0.1"

#define WINDOW_WIDTH 400
#define WINDOW_HEIGHT 300

#define BUTTON_COUNT 5

const char *button_labels[BUTTON_COUNT] = { "Feed", "Read", "Sleep", "Hug", "Play" };
const char button_chars[BUTTON_COUNT] = { 'F', 'R', 'S', 'H', 'P' };

typedef struct {
    SDL_Rect rect;
    const char *label;
    char send_char;
} Button;

void render_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y) {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text, white);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dst = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &dst);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }
    if (TTF_Init() != 0) {
        printf("TTF_Init error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Client GUI", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    TTF_Font *font = TTF_OpenFont("FreeSans.ttf", 16); 
    if (!font) {
        printf("Failed to load font: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    Button buttons[BUTTON_COUNT];
    int btn_width = 70, btn_height = 40;
    int spacing = 10;
    for (int i = 0; i < BUTTON_COUNT; i++) {
        buttons[i].rect.x = spacing + i * (btn_width + spacing);
        buttons[i].rect.y = WINDOW_HEIGHT / 2 - btn_height / 2;
        buttons[i].rect.w = btn_width;
        buttons[i].rect.h = btn_height;
        buttons[i].label = button_labels[i];
        buttons[i].send_char = button_chars[i];
    }

    // Socket setup
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in serv_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT)
    };
    inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    int running = 1;
    char last_partner_action = 0;

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = 0;
            } else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int mx = e.button.x;
                int my = e.button.y;
                for (int i = 0; i < BUTTON_COUNT; i++) {
                    SDL_Rect r = buttons[i].rect;
                    if (mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h) {
                        // Kliknięto guzik
                        char to_send = buttons[i].send_char;
                        send(sock, &to_send, 1, 0);

                        // Czekaj na odpowiedź partnera
                        char received;
                        int ret = recv(sock, &received, 1, 0);
                        if (ret <= 0) {
                            printf("Błąd odbioru od serwera.\n");
                            running = 0;
                            break;
                        }
                        last_partner_action = received;
                    }
                }
            }
        }

        // Rysowanie GUI
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Rysuj przyciski
        for (int i = 0; i < BUTTON_COUNT; i++) {
            SDL_SetRenderDrawColor(renderer, 70, 130, 180, 255);
            SDL_RenderFillRect(renderer, &buttons[i].rect);
            render_text(renderer, font, buttons[i].label,
                        buttons[i].rect.x + 10, buttons[i].rect.y + 10);
        }

        // Wyświetl ostatnią akcję partnera
        if (last_partner_action) {
            char buf[64];
            snprintf(buf, sizeof(buf), "Partner wybrał: %c", last_partner_action);
            render_text(renderer, font, buf, 20, 20);
        } else {
            render_text(renderer, font, "Kliknij przycisk, aby wysłać akcję", 20, 20);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    close(sock);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
