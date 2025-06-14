#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "creature.h"
#include "protocol.h"
#include "cJSON.h"

#define WINDOW_WIDTH 840
#define WINDOW_HEIGHT 480

#define BUTTON_COUNT 5

SDL_Rect buttons[BUTTON_COUNT];

const char *button_labels[BUTTON_COUNT] = { "Feed", "Read", "Sleep", "Hug", "Play" };

// --- Networking ---
int sockfd = -1;
int session_id = -1;
int player_id = 0;
bool running = true;
char game_id[64] = {0};
pthread_mutex_t session_mutex = PTHREAD_MUTEX_INITIALIZER;

// Obsługa guzików
int last_clicked_button = -1;
Uint32 last_click_time = 0;

// Prototypy funkcji
void draw_bar(SDL_Renderer *r, int x, int y, int w, int h, int value, SDL_Color color);
void draw_text(SDL_Renderer *r, TTF_Font *font, const char *text, int x, int y, SDL_Color color);
void draw_ascii_art(SDL_Renderer *r, TTF_Font *font, const char *ascii_art, int x, int y, SDL_Color color);
char* load_ascii_art_from_file(const char *filepath);
void draw_buttons(SDL_Renderer *renderer, TTF_Font *font_regular, TTF_Font *font_emoji, SDL_Color color);
int check_button_click(int x, int y);

void send_message(Message *msg) {
    char json_str[MAX_MSG_LEN];
    snprintf(json_str, MAX_MSG_LEN,
        "{\"type\":%d,\"session_id\":%d,\"player_id\":%d,\"action_code\":%d,\"payload\":\"%s\"}\n",
        msg->type, msg->session_id, msg->player_id, msg->action_code, msg->payload);

    send(sockfd, json_str, strlen(json_str), 0);
}

// Minimalna pusta obsługa przychodzących wiadomości
void handle_server_message(const char *json_str) {
    (void)json_str;  // ignorujemy
}

void *receive_thread(void *arg) {
    char buffer[4096];
    int buf_len = 0;

    while (running) {
        char chunk[512];
        int len = recv(sockfd, chunk, sizeof(chunk) - 1, 0);
        if (len <= 0) {
            running = false;
            break;
        }
        chunk[len] = '\0';

        // dodaj do bufora
        if (buf_len + len < sizeof(buffer)) {
            memcpy(buffer + buf_len, chunk, len);
            buf_len += len;
            buffer[buf_len] = '\0';

            char *line_start = buffer;
            char *newline;
            while ((newline = strchr(line_start, '\n')) != NULL) {
                *newline = '\0';
                handle_server_message(line_start);
                line_start = newline + 1;
            }

            buf_len = strlen(line_start);
            memmove(buffer, line_start, buf_len);
            buffer[buf_len] = '\0';
        } else {
            buf_len = 0;
        }
    }
    return NULL;
}

int check_button_click(int x, int y) {
    for (int i = 0; i < BUTTON_COUNT; i++) {
        SDL_Rect r = buttons[i];
        if (x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h)
            return i;
    }
    return -1;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        return 1;
    }

    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    if (SDL_Init(SDL_INIT_VIDEO) < 0 || TTF_Init() < 0) {
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Pairents",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    TTF_Font *font_text  = TTF_OpenFont("client/assets/fonts/MatrixtypeDisplayBold-6R4e6.ttf", 14);
    TTF_Font *font_emoji = TTF_OpenFont("client/assets/fonts/NotoEmoji-VariableFont_wght.ttf", 18);
    TTF_Font *font_bunny = TTF_OpenFont("client/assets/fonts/MatrixtypeDisplayBold-6R4e6.ttf", 6);

    if (!font_text || !font_emoji || !font_bunny) {
        return 1;
    }

    // Pomijam ładowanie ascii art i creature, pozostawiam jeśli chcesz zostawić GUI

    // Socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return 1;
    }

    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        return 1;
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        return 1;
    }

    pthread_t recv_tid;
    pthread_create(&recv_tid, NULL, receive_thread, NULL);

    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int clicked = check_button_click(event.button.x, event.button.y);
                if (clicked != -1) {
                    pthread_mutex_lock(&session_mutex);
                    bool can_click = (session_id != -1) && (game_id[0] != '\0');
                    pthread_mutex_unlock(&session_mutex);

                    if (can_click) {
                        last_clicked_button = clicked;
                        last_click_time = SDL_GetTicks();

                        Message msg = {0};
                        msg.type = MSG_ACTION;
                        pthread_mutex_lock(&session_mutex);
                        msg.session_id = session_id;
                        msg.player_id = player_id;
                        pthread_mutex_unlock(&session_mutex);
                        msg.action_code = clicked;
                        snprintf(msg.payload, sizeof(msg.payload), "%s", button_labels[clicked]);

                        send_message(&msg);
                    }
                }
            }
        }

        // Rysowanie GUI i inne rzeczy pozostają bez zmian (lub możesz to uprościć/wywalić)
        SDL_Delay(16);
    }

    running = false;
    pthread_join(recv_tid, NULL);

    close(sockfd);
    TTF_CloseFont(font_text);
    TTF_CloseFont(font_emoji);
    TTF_CloseFont(font_bunny);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
