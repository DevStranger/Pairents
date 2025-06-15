#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <SDL2/SDL.h>

#include "gui.h"

#define PORT 12345
#define SERVER_IP "127.0.0.1"

int connect_to_server() {
    // ... (bez zmian, jak w oryginale)
}

int main(int argc, char *argv[]) {
    GUI gui;
    if (gui_init(&gui) != 0) {
        return 1;
    }

    int sock = connect_to_server();
    if (sock < 0) {
        gui_destroy(&gui);
        return 1;
    }

    gui_draw_buttons(&gui);

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
                switch (response[1]) {
                    case 0:
                        printf("Wynik: różne wybory (mismatch).\n");
                        break;
                    case 1:
                        printf("Wynik: takie same wybory (accepted).\n");
                        switch (response[0]) {
                            case 0: printf("Fed\n"); break;
                            case 1: printf("Read\n"); break;
                            case 2: printf("Slept\n"); break;
                            case 3: printf("Hugged\n"); break;
                            case 4: printf("Played\n"); break;
                            default: printf("Nieznana akcja\n");
                        }
                        break;
                    case 2:
                        printf("Wynik: oczekiwanie na drugiego gracza.\n");
                        break;
                    default:
                        printf("Nieznany status.\n");
                }
                waiting_for_response = 0;
                gui_draw_buttons(&gui);
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

        SDL_Delay(10);
    }

    close(sock);
    gui_destroy(&gui);
    return 0;
}
