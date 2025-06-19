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

char *current_ascii_art = NULL;       // aktualny ascii art do wyświetlenia
Uint32 temp_art_end_time = 0;         // timestamp, do kiedy wyświetlać temp art
char *default_ascii_art = NULL;       // domyślny ascii art
int waiting_for_creature = 0;         // zmienna globalna

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

int main(int argc, char *argv[]) {
    GUI gui;
    if (gui_init(&gui) != 0) return 1;

    char message[128] = "";            // na wiadomości od serwera
    Uint32 message_expire_time = 0;    // czas wyświetlania wiadomości
    
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

    default_ascii_art = load_ascii_art("assets/default.txt");
    if (!default_ascii_art) {
        fprintf(stderr, "Nie udało się załadować domyślnego ASCII art\n");
        gui_destroy(&gui);
        return 1;
    }
    current_ascii_art = default_ascii_art;  // na start wyświetlamy domyślny art
    
    Creature creature = {
        .hunger = 70,
        .happiness = 80,
        .sleep = 60,
        .health = 90,
        .growth = 50,
        .love = 75,
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
            ssize_t received = recv(sock, response, 2, 0);
            if (received == 2) {
                unsigned char partner_choice = response[0];
                unsigned char status = response[1];

                switch (status) {
                    case 0:
                        printf("Wynik: różne wybory (mismatch).\n");
                        snprintf(message, sizeof(message), "Mismatch :( Try again!");
                        message_expire_time = SDL_GetTicks() + 8000;
                        waiting_for_response = 0;
                        break;
                    case 1:
                        printf("Wynik: takie same wybory (accepted).\n");
                        snprintf(message, sizeof(message), "Matched!");
                        message_expire_time = SDL_GetTicks() + 8000;
                        const char *action_ascii_files[] = {
                            "assets/fed.txt",
                            "assets/read.txt",
                            "assets/slept.txt",
                            "assets/hugged.txt",
                            "assets/played.txt"
                        };
                        if (partner_choice <= 4) {
                            char *new_art = load_ascii_art(action_ascii_files[partner_choice]);
                            if (new_art) {
                                if (current_ascii_art && current_ascii_art != default_ascii_art) {
                                    free(current_ascii_art);
                                }
                                current_ascii_art = new_art;
                                temp_art_end_time = SDL_GetTicks() + 4000; // wyświetlaj przez 4 sekundy
                            } else {
                                fprintf(stderr, "Nie udało się załadować pliku ASCII art: %s\n", action_ascii_files[partner_choice]);
                                snprintf(message, sizeof(message), "Bunny got lost :(");
                                message_expire_time = SDL_GetTicks() + 8000;
                            }
                        }

                        // Rozpocznij odbiór nieblokujący stwora
                        reset_creature_receiver();
                        waiting_for_creature = 1;
                        waiting_for_response = 0;
                        break;
                    case 2:
                        printf("Wynik: oczekiwanie na drugiego gracza.\n");
                        snprintf(message, sizeof(message), "Waiting ...");
                        message_expire_time = SDL_GetTicks() + 45000;
                        break;
                    default:
                        printf("Nieznany status.\n");
                        snprintf(message, sizeof(message), "Unknown status");
                        message_expire_time = SDL_GetTicks() + 8000;
                        waiting_for_response = 0;
                }
            } else if (received == 0) {
                printf("Serwer zamknął połączenie.\n");
                running = 0;
            } else if (received == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                // brak danych
            } else {
                perror("recv");
                running = 0;
            }
        }

        // ODBIÓR STWORA W KOLEJNYCH KLATKACH
       if (waiting_for_creature) {
            int result = creature_recv_partial(sock, &creature);
            if (result == 1) {
                printf("Stan stwora zaktualizowany.\n");
                waiting_for_creature = 0;
                waiting_for_response = 0; 
            } else if (result == -1) {
                fprintf(stderr, "Błąd podczas odbierania stwora.\n");
                running = 0;
            }
        }

        update_creature(&creature);

        Uint32 now = SDL_GetTicks();
        if (current_ascii_art != default_ascii_art && now > temp_art_end_time) {
            free(current_ascii_art);
            current_ascii_art = default_ascii_art;
            temp_art_end_time = 0;
        }

        //SDL_SetRenderDrawColor(gui.renderer, 0, 0, 0, 255);
        //SDL_RenderClear(gui.renderer);
        gui_draw_buttons(&gui, &creature, current_ascii_art, font_text, font_emoji);
        if (SDL_GetTicks() < message_expire_time) {
            gui_draw_message(&gui, message, font_text);
        }
        if (SDL_GetTicks() >= message_expire_time) {
            message[0] = '\0';
        }
        SDL_RenderPresent(gui.renderer);

        SDL_Delay(16);
    }

    close(sock);
    TTF_CloseFont(font_text);
    TTF_CloseFont(font_emoji);
    if (current_ascii_art && current_ascii_art != default_ascii_art) {
    free(current_ascii_art);
    }
    if (default_ascii_art) {
        free(default_ascii_art);
    }
    gui_destroy(&gui);
    return 0;
}
