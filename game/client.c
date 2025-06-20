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

// zmienne globalne
char *current_ascii_art = NULL;       // aktualny ascii art do wyświetlenia
Uint32 temp_art_end_time = 0;         // timestamp, do kiedy wyświetlać current art (as temporary - czasowy)
char *default_ascii_art = NULL;       // domyślny ascii art
int waiting_for_creature = 0;         // flaga oznaczająca oczekiwanie na dane o stworze

// funkcja łącząca się z serwerem pod podanym IP i portem
int connect_to_server(const char *ip, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);        // tworzymy gniazdo TCP
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in serv_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port)                    // zmiana portu na sieciowy format
    };

    // konwersja ip na binary
    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        return -1;
    }

    // łączenie z serwerem
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        close(sock);
        return -1;
    }

    // tryb nieblokujący, aby recv nie blokował pętli głównej
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

    return sock;                 // zwracamy deskryptor socketu
}

// funkcja ładująca zawartość pliku ASCII art do pamięci
char *load_ascii_art(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("fopen ascii_art");
        return NULL;
    }

    fseek(file, 0, SEEK_END);           // ustawiamy wskaźnik na koniec pliku
    long size = ftell(file);            // pobieramy rozmiar pliku
    rewind(file);                       // wracamy na początek pliku

    char *buffer = malloc(size + 1);    // alokujemy pamięć na zawartość + znak końca stringa
    if (!buffer) {
        fclose(file);
        fprintf(stderr, "malloc ascii_art failed\n");
        return NULL;
    }

    fread(buffer, 1, size, file);        // czytamy całą zawartość pliku
    buffer[size] = '\0';                 // dodajemy znak końca stringa
    fclose(file);
    return buffer;
}

int main(int argc, char *argv[]) {
    GUI gui;
    if (gui_init(&gui) != 0) return 1;     // inicjalizacja SDL i GUI

    char message[128] = "";                // na wiadomości od serwera
    Uint32 message_expire_time = 0;        // czas wyświetlania wiadomości
    
    if (TTF_Init() == -1) {
        fprintf(stderr, "TTF_Init Error: %s\n", TTF_GetError());
        gui_destroy(&gui);
        return 1;
    }

    // ładowanie fontów - tekst i emoji
    TTF_Font *font_text = TTF_OpenFont("../assets/MatrixtypeDisplayBold-6R4e6.ttf", 20);
    if (!font_text) {
        fprintf(stderr, "TTF_OpenFont font_text failed: %s\n", TTF_GetError());
        gui_destroy(&gui);
        return 1;
    }
    TTF_Font *font_emoji = TTF_OpenFont("../assets/NotoEmoji-VariableFont_wght.ttf", 24);
    if (!font_emoji) {
        fprintf(stderr, "TTF_OpenFont font_emoji failed: %s\n", TTF_GetError());
        TTF_CloseFont(font_text);
        gui_destroy(&gui);
        return 1;
    }

    // ładujemy domyślny ASCII art z pliku
    default_ascii_art = load_ascii_art("../assets/default.txt");
    if (!default_ascii_art) {
        fprintf(stderr, "Nie udało się załadować domyślnego ASCII art\n");
        gui_destroy(&gui);
        return 1;
    }
    current_ascii_art = default_ascii_art;  // na start wyświetlamy domyślny art

    // inicjalizacja stwora
    Creature creature = {};
    
    if (argc != 3) {
        fprintf(stderr, "Użycie: %s <IP_serwera> <PORT>\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    if (server_port <= 0 || server_port > 65535) {
        fprintf(stderr, "Niepoprawny numer portu: %s\n", argv[2]);
        return 1;
    }

    // nawiązujemy połączenie z serwerem    
    int sock = connect_to_server(server_ip, server_port);
    
    if (sock < 0) {
        TTF_CloseFont(font_text);
        TTF_CloseFont(font_emoji);
        gui_destroy(&gui);
        return 1;
    }

    int running = 1;                    // flaga pętli głównej programu
    int waiting_for_response = 0;       // flaga oczekiwania na odpowiedź serwera
    SDL_Event e;                        // struktura do obsługi zdarzeń SDL

    while (running) {
        // obsługa zdarzeń SDL (klik klik, zamykanie okna, etc.)
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = 0;
            } else if (!waiting_for_response &&
                       e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int button_index = gui_check_button_click(&gui, e.button.x, e.button.y);
                if (button_index >= 0) {
                    unsigned char choice = (unsigned char)button_index;
                    // wysyłamy info o klikniętym guziku do serwera
                    if (send(sock, &choice, 1, 0) != 1) {
                        perror("send");
                        running = 0;
                        break;
                    }
                    waiting_for_response = 1;        // czekamy teraz na odpowiedź od serwera
                }
            }
        }

        // jeśli czekamy na odpowiedź serwera, to musimy ją teź od niego odebrać
        if (waiting_for_response) {
            unsigned char response[2];
            ssize_t received = recv(sock, response, 2, 0);
            if (received == 2) {
                unsigned char partner_choice = response[0];    // wybór partnera
                unsigned char status = response[1];            // status synchronizacji akcji

                switch (status) {
                    case 0:                                    // różne wybory 
                        printf("Wynik: różne wybory (mismatch).\n");
                        snprintf(message, sizeof(message), "Mismatch :( Try again!");
                        message_expire_time = SDL_GetTicks() + 8000;
                        waiting_for_response = 0;
                        break;
                    case 1:                                    // ten sam wybór
                        printf("Wynik: takie same wybory (accepted).\n");
                        snprintf(message, sizeof(message), "Matched!");
                        message_expire_time = SDL_GetTicks() + 8000;
                        // tablica ścieżek do plików ASCII art odpowiadających akcjom
                        const char *action_ascii_files[] = {
                            "../assets/fed.txt",
                            "../assets/read.txt",
                            "../assets/slept.txt",
                            "../assets/hugged.txt",
                            "../assets/played.txt"
                        };
                        if (partner_choice <= 4) {
                            char *new_art = load_ascii_art(action_ascii_files[partner_choice]);
                            if (new_art) {
                                // jeśli jest już załadowany inny art tymczasowy, zwalniamy go
                                if (current_ascii_art && current_ascii_art != default_ascii_art) {
                                    free(current_ascii_art);
                                }
                                current_ascii_art = new_art;
                                temp_art_end_time = SDL_GetTicks() + 4000; // wyświetlamy przez 4 sekundy
                            } else {
                                fprintf(stderr, "Nie udało się załadować pliku ASCII art: %s\n", action_ascii_files[partner_choice]);
                                snprintf(message, sizeof(message), "Bunny got lost :(");
                                message_expire_time = SDL_GetTicks() + 8000;
                            }
                        }

                        // rozpoczynamy odbiór nieblokujący stwora
                        reset_creature_receiver();
                        waiting_for_creature = 1;
                        waiting_for_response = 0;
                        break;
                    case 2:                                                // czekamy na drugiego gracza, bo się nie może zdecydować na jeden z pięciu guzików xD
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
                // brak danych - kontynuujemy pętle
            } else {
                perror("recv");
                running = 0;
            }
        }

       // odbieramy częściowo dane o stanie stwora co by nie zatrzymwać całej pętli programu
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

        // aktualizacja stanu stwora lokalnie
        update_creature(&creature);

        // powrót do domyślnego image-u stwora
        Uint32 now = SDL_GetTicks();
        if (current_ascii_art != default_ascii_art && now > temp_art_end_time) {
            free(current_ascii_art);
            current_ascii_art = default_ascii_art;
            temp_art_end_time = 0;
        }

        // rysowanie gui
        gui_draw_buttons(&gui, &creature, current_ascii_art, current_ascii_art == default_ascii_art, font_text, font_emoji);

        // wiadomości
        if (SDL_GetTicks() < message_expire_time) {
            gui_draw_message(&gui, message, font_text);
        }
        if (SDL_GetTicks() >= message_expire_time) {
            message[0] = '\0';
        }
        SDL_RenderPresent(gui.renderer);

        SDL_Delay(16);                    // delay ~ 60fps
    }

    // sprzątanie przed zamknięciem lokalu
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
