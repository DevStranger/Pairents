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

void set_temp_ascii_art(Creature *c, const char *filename, Uint32 duration_ms);

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

char *load_ascii_art(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("fopen ascii_art");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char *buffer = malloc(size + 1);
    if (!buffer) {
        fclose(file);
        fprintf(stderr, "malloc ascii_art failed\n");
        return NULL;
    }

    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    fclose(file);
    return buffer;
}

void set_temp_ascii_art(Creature *c, const char *filename, Uint32 duration_ms) {
    if (c->temp_ascii_art) {
        free(c->temp_ascii_art);
        c->temp_ascii_art = NULL;
    }
    c->temp_ascii_art = load_ascii_art(filename);
    if (c->temp_ascii_art) {
        c->temp_art_end_time = SDL_GetTicks() + duration_ms;
    } else {
        c->temp_art_end_time = 0;
    }
}

    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    fclose(file);
    return buffer;
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
        .love = 75,
        .ascii_art = load_ascii_art("assets/default.txt")
    };

    if (!creature.ascii_art) {
        fprintf(stderr, "Nie udało się załadować ASCII art\n");
        TTF_CloseFont(font_text);
        TTF_CloseFont(font_emoji);
        gui_destroy(&gui);
        return 1;
    }

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
            ssize_t received = recv(sock, response, 2, 0);
            if (received == 2) {
                unsigned char partner_choice = response[0];
                unsigned char status = response[1];
        
                switch (status) {
                    case 0:
                        printf("Wynik: różne wybory (mismatch).\n");
                        break;
                    case 1:
                        printf("Wynik: takie same wybory (accepted).\n");
                        const char *action_ascii_files[] = {
                            "assets/fed.txt",
                            "assets/read.txt",
                            "assets/slept.txt",
                            "assets/hugged.txt",
                            "assets/played.txt"
                        };
                        
                        if (partner_choice <= 4) {
                            printf("Akcja: %s\n", action_ascii_files[partner_choice]);
                            set_temp_ascii_art(&creature, action_ascii_files[partner_choice], 8000); // 8 sekund
                        }

                        // Odbierz stan stwora od serwera
                        {
                            Creature new_creature;
                            ssize_t creature_received = 0;
                            char *ptr = (char *)&new_creature;
                            size_t to_receive = sizeof(Creature);
        
                            // Blokujący odbiór całej struktury (możesz rozważyć timeout lub inny sposób)
                            while (to_receive > 0) {
                                ssize_t r = recv(sock, ptr + creature_received, to_receive, 0);
                                if (r <= 0) {
                                    if (r == 0) {
                                        printf("Serwer zamknął połączenie podczas odbierania stwora.\n");
                                        running = 0;
                                        break;
                                    }
                                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                        // Nie ma jeszcze danych, kontynuuj pętlę
                                        SDL_Delay(1);
                                        continue;
                                    }
                                    perror("recv stwora");
                                    running = 0;
                                    break;
                                }
                                creature_received += r;
                                to_receive -= r;
                            }
                            if (running) {
                                creature = new_creature;
                            }
                        }
                        break;
                    case 2:
                        printf("Wynik: oczekiwanie na drugiego gracza.\n");
                        break;
                    default:
                        printf("Nieznany status.\n");
                }
                waiting_for_response = 0;
            } else if (received == 0) {
                printf("Serwer zamknął połączenie.\n");
                running = 0;
            } else if (received == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                // brak danych, nic nie rób
            } else {
                perror("recv");
                running = 0;
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
    free(creature.ascii_art);
    gui_destroy(&gui);
    return 0;
}
