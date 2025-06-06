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

// --- GUI ---
#define BUTTON_COUNT 5
SDL_Rect buttons[BUTTON_COUNT];
const char *button_labels[BUTTON_COUNT] = {"Feed", "Read", "Sleep", "Hug", "Play"};

// --- Networking ---
int sockfd = -1;
int session_id = -1;
int player_id = 0;  
bool running = true;

// ASCII art
char *bunny_art = NULL;

// Prototypy funkcji
void draw_bar(SDL_Renderer *r, int x, int y, int w, int h, int value, SDL_Color color);
void draw_text(SDL_Renderer *r, TTF_Font *font, const char *text, int x, int y, SDL_Color color);
void draw_ascii_art(SDL_Renderer *r, TTF_Font *font, const char *ascii_art, int x, int y, SDL_Color color);
char* load_ascii_art_from_file(const char *filepath);
void draw_buttons(SDL_Renderer *renderer, TTF_Font *font, SDL_Color color);
int check_button_click(int x, int y);

// --- Sieć: wysyłanie i odbieranie ---
void send_message(Message *msg) {
    char json_str[MAX_MSG_LEN];
    snprintf(json_str, MAX_MSG_LEN,
        "{\"type\":%d,\"session_id\":%d,\"player_id\":%d,\"action_code\":%d,\"payload\":\"%s\"}",
        msg->type, msg->session_id, msg->player_id, msg->action_code, msg->payload);

    send(sockfd, json_str, strlen(json_str), 0);
}

void handle_server_message(const char *json_str) {
    cJSON *json = cJSON_Parse(json_str);
    if (!json) {
        printf("[CLIENT] Niepoprawny JSON od serwera: %s\n", json_str);
        return;
    }

    cJSON *type_json = cJSON_GetObjectItemCaseSensitive(json, "type");
    cJSON *session_json = cJSON_GetObjectItemCaseSensitive(json, "session_id");
    cJSON *payload_json = cJSON_GetObjectItemCaseSensitive(json, "payload");

    if (!cJSON_IsNumber(type_json)) {
        cJSON_Delete(json);
        return;
    }

    int msg_type = type_json->valueint;

    switch (msg_type) {
        case MSG_PAIR:
            if (cJSON_IsNumber(session_json)) {
                session_id = session_json->valueint;
                printf("[CLIENT] Połączono w sesję ID=%d\n", session_id);
            }
            if (cJSON_IsString(payload_json)) {
                printf("[CLIENT] Game ID: %s\n", payload_json->valuestring);
            }
            break;

        case MSG_ACTION:
            if (cJSON_IsString(payload_json)) {
                printf("[CLIENT] Otrzymano akcję drugiego gracza: %s\n", payload_json->valuestring);
            }
            break;

        case MSG_RESULT:
            if (cJSON_IsString(payload_json)) {
                printf("[CLIENT] Wynik: %s\n", payload_json->valuestring);
            }
            break;

        default:
            printf("[CLIENT] Nieznany typ wiadomości: %d\n", msg_type);
            break;
    }

    cJSON_Delete(json);
}

void *receive_thread(void *arg) {
    char buffer[MAX_MSG_LEN];
    while (running) {
        int len = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (len <= 0) {
            printf("[CLIENT] Połączenie zerwane lub błąd odbioru.\n");
            running = false;
            break;
        }
        buffer[len] = '\0';
        handle_server_message(buffer);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Użycie: %s <adres_serwera> <port>\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    // Inicjalizacja SDL i TTF
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || TTF_Init() < 0) {
        fprintf(stderr, "Błąd inicjalizacji SDL lub TTF: %s\n", SDL_GetError());
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
        printf("Brak czcionki! %s\n", TTF_GetError());
        return 1;
    }

    bunny_art = load_ascii_art_from_file("client/assets/bunny/default.txt");
    if (!bunny_art) {
        return 1;
    }

    // Socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        fprintf(stderr, "Niepoprawny adres IP\n");
        return 1;
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        return 1;
    }
    printf("[CLIENT] Połączono z serwerem %s:%d\n", server_ip, server_port);

    // Start odbioru w osobnym wątku
    pthread_t recv_tid;
    pthread_create(&recv_tid, NULL, receive_thread, NULL);

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

    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int clicked = check_button_click(event.button.x, event.button.y);
                if (clicked != -1 && session_id != -1) {
                    printf("Kliknięto: %s\n", button_labels[clicked]);

                    // Wyślij akcję do serwera
                    Message msg = {0};
                    msg.type = MSG_ACTION;
                    msg.session_id = session_id;
                    msg.player_id = player_id;
                    msg.action_code = clicked;  
                    snprintf(msg.payload, sizeof(msg.payload), "%s", button_labels[clicked]);

                    send_message(&msg);
                }
            }
        }

        Uint32 now = SDL_GetTicks();
        if (now - last_update > 1000) {
            update_creature(&creature);
            last_update = now;
        }

        // Rysowanie GUI
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
        draw_bar(renderer, 155, 162, 200, 16, creature.growth, pink);
        char cuddle_value[8];
        sprintf(cuddle_value, "%d%%", creature.growth);
        draw_text(renderer, font_text, cuddle_value, 360, 160, white);

        draw_text(renderer, font_emoji, "❤️", 20, 190, white);
        draw_text(renderer, font_text, "Love", 60, 190, white);
        draw_bar(renderer, 155, 192, 200, 16, creature.love, orange);
        char magic_value[8];
        sprintf(magic_value, "%d%%", creature.love);
        draw_text(renderer, font_text, magic_value, 360, 190, white);

        // Rysuj ASCII art (stworzonko)
        draw_ascii_art(renderer, font_bunny, bunny_art, 450, 50, white);

        // Rysuj przyciski
        draw_buttons(renderer, font_emoji, white);

        SDL_RenderPresent(renderer);

        SDL_Delay(16);  // ok. 60 FPS
    }

    running = false;
    pthread_join(recv_tid, NULL);

    free(bunny_art);

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

// --- Funkcje pomocnicze ---

char* load_ascii_art_from_file(const char *filepath) {
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

void draw_bar(SDL_Renderer *r, int x, int y, int w, int h, int value, SDL_Color color) {
    SDL_Rect bg = {x, y, w, h};
    SDL_SetRenderDrawColor(r, 50, 50, 50, 255);
    SDL_RenderFillRect(r, &bg);

    SDL_Rect fg = {x, y, w * value / 100, h};
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, 255);
    SDL_RenderFillRect(r, &fg);
}

void draw_text(SDL_Renderer *r, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(r, surface);
    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(r, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void draw_ascii_art(SDL_Renderer *r, TTF_Font *font, const char *ascii_art, int x, int y, SDL_Color color) {
    int line_height = TTF_FontHeight(font);
    const char *line_start = ascii_art;
    int line_num = 0;

    while (*line_start) {
        const char *line_end = strchr(line_start, '\n');
        int len = line_end ? (line_end - line_start) : strlen(line_start);

        char buffer[256];
        strncpy(buffer, line_start, len);
        buffer[len] = '\0';

        draw_text(r, font, buffer, x, y + line_num * line_height, color);

        if (!line_end) break;
        line_start = line_end + 1;
        line_num++;
    }
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
        SDL_Rect r = buttons[i];
        if (x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h)
            return i;
    }
    return -1;
}
