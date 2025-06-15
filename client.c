#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <SDL2/SDL.h>

#include "gui.h"
#include "creature.h"

#define PORT 12345
#define SERVER_IP "127.0.0.1"

int connect_to_server() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in serv_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT)
    };
    inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        close(sock);
        return -1;
    }

    // Ustaw socket w tryb nieblokujący
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl get");
        close(sock);
        return -1;
    }
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl set");
        close(sock);
        return -1;
    }

    return sock;
}

void apply_action(Creature *c, unsigned char action) {
    switch (action) {
        case 0: // Fed
            c->hunger += 15;
            if (c->hunger > 100) c->hunger = 100;
            break;
        case 1: // Read
            c->growth += 10;
            if (c->growth > 100) c->growth = 100;
            break;
        case 2: // Slept
            c->sleep += 20;
            if (c->sleep > 100) c->sleep = 100;
            break;
        case 3: // Hugged
            c->love += 15;
            if (c->love > 100) c->love = 100;
            break;
        case 4: // Played
            c->happiness += 15;
            if (c->happiness > 100) c->happiness = 100;
            break;
        default:
            break;
    }
}

int main(int argc, char *argv[]) {
    GUI gui;
    if (gui_init(&gui) != 0) {
        return 1;
    }

    // Asynchroniczna aktualizacja stanu stwora
    typedef struct {
        char buffer[sizeof(Creature)];
        size_t received;
        int receiving_creature; // flaga czy jesteśmy w trakcie odbierania stwora
    } ReceiveState;

    ReceiveState recv_state = { .received = 0, .receiving_creature = 0 };

    if (TTF_Init() == -1) {
        fprintf(stderr, "TTF_Init Error: %s\n", TTF_GetError());
        gui_destroy(&gui);
        return 1;
    }

    TTF_Font *font_text = TTF_OpenFont("assets/MatrixtypeDisplayBold-6R4e6.ttf", 20);
    if (!font_text) {
        fprintf(stderr, "TTF_OpenFont font_text failed: %s\n", TTF_GetError());
        gui_destroy(&gui);
        return 1;
    }
    TTF_Font *font_emoji = TTF_OpenFont("assets/NotoEmoji-VariableFont_wght.ttf", 24);
    if (!font_emoji) {
        fprintf(stderr, "TTF_OpenFont font_emoji failed: %s\n", TTF_GetError());
        TTF_CloseFont(font_text);
        gui_destroy(&gui);
        return 1;
    }

    Creature creature = {
        .hunger = 70,
        .happiness = 80,
        .sleep = 60,
        .health = 90,
        .growth = 50,
        .love = 75
    };

    int sock = connect_to_server();
    if (sock < 0) {
        TTF_CloseFont(font_text);
        TTF_CloseFont(font_emoji);
        gui_destroy(&gui);
        return 1;
    }

    int running = 1;
    int waiting_for_response = 0;
    SDL_Event e;

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = 0;
            } else if (!waiting_for_response &&
                       e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int button_index = gui_check_button_click(&gui, e.button.x, e.button.y);
                if (button_index >= 0) {
                    unsigned char choice = (unsigned char)button_index;
                    if (send(sock, &choice, 1, 0) != 1) {
                        perror("send");
                        running = 0;
                        break;
                    }
                    waiting_for_response = 1;
                }
            }
        }

        if (waiting_for_response) {
            unsigned char response[2];
            if (!recv_state.receiving_creature) {
                ssize_t received = recv(sock, response, 2, 0);
                if (received == 2) {
                    unsigned char partner_choice = response[0];
                    unsigned char status = response[1];
            
                    switch (status) {
                        case 0:
                            printf("Wynik: różne wybory (mismatch).\n");
                            waiting_for_response = 0;
                            break;
                        case 1:
                            printf("Wynik: takie same wybory (accepted).\n");
                            // Włącz tryb odbioru stwora
                            recv_state.receiving_creature = 1;
                            recv_state.received = 0;
                            break;
                        case 2:
                            printf("Wynik: oczekiwanie na drugiego gracza.\n");
                            waiting_for_response = 0;
                            break;
                        default:
                            printf("Nieznany status.\n");
                            waiting_for_response = 0;
                    }
                } else if (received == 0) {
                    printf("Serwer zamknął połączenie.\n");
                    running = 0;
                } else if (received == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    // brak danych - nic nie rób
                } else {
                    perror("recv");
                    running = 0;
                }
            } else {
                // Odbieramy Creature kawałek po kawałku
                ssize_t r = recv(sock, recv_state.buffer + recv_state.received, sizeof(Creature) - recv_state.received, 0);
                if (r > 0) {
                    recv_state.received += r;
                    if (recv_state.received == sizeof(Creature)) {
                        // Pełny stan stwora odebrany
                        memcpy(&creature, recv_state.buffer, sizeof(Creature));
                        recv_state.receiving_creature = 0;
                        waiting_for_response = 0;
                        printf("Stan stwora zaktualizowany.\n");
                    }
                } else if (r == 0) {
                    printf("Serwer zamknął połączenie podczas odbioru stwora.\n");
                    running = 0;
                } else if (r == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    // brak danych - nic nie rób
                } else {
                    perror("recv Creature");
                    running = 0;
                }
            }
        }
        
        // Aktualizuj stan stwora co iterację
        update_creature(&creature);

        // Czyść ekran (tutaj zakładam, że gui.renderer jest SDL_Renderer)
        SDL_SetRenderDrawColor(gui.renderer, 0, 0, 0, 255); // czarny
        SDL_RenderClear(gui.renderer);

        // Rysuj GUI z aktualnym stanem
        gui_draw_buttons(&gui, &creature, font_text, font_emoji);

        // Wyświetl nowy frame
        SDL_RenderPresent(gui.renderer);

        SDL_Delay(16); // ok. 60 FPS
    }

    close(sock);
    TTF_CloseFont(font_text);
    TTF_CloseFont(font_emoji);
    gui_destroy(&gui);
    return 0;
}
