#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <SDL2/SDL.h>

#define PORT 12345
#define SERVER_IP "127.0.0.1"
#define WINDOW_WIDTH 500
#define WINDOW_HEIGHT 200
#define BUTTON_WIDTH 90
#define BUTTON_HEIGHT 60

const char *button_labels[] = { "Feed", "Read", "Sleep", "Hug", "Play" };

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

    return sock;
}

int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("Wybierz akcję", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!win) {
        fprintf(stderr, "SDL CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!ren) {
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    SDL_Rect buttons[5];
    for (int i = 0; i < 5; ++i) {
        buttons[i].x = 20 + i * (BUTTON_WIDTH + 10);
        buttons[i].y = 50;
        buttons[i].w = BUTTON_WIDTH;
        buttons[i].h = BUTTON_HEIGHT;
    }

    int sock = connect_to_server();
    if (sock < 0) {
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    int running = 1;
    int choice_pending = 0;  // flaga czy wysłaliśmy wybór i czekamy na odpowiedź
    unsigned char button_clicked = 0;

    SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = 0;
            } else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                if (!choice_pending) {  // tylko jeśli nie czekamy na odpowiedź
                    int mx = e.button.x;
                    int my = e.button.y;
                    for (int i = 0; i < 5; ++i) {
                        if (mx >= buttons[i].x && mx <= buttons[i].x + buttons[i].w &&
                            my >= buttons[i].y && my <= buttons[i].y + buttons[i].h) {
                            button_clicked = i;
                            choice_pending = 1;  // zaznaczamy, że czekamy na odpowiedź
                            break;
                        }
                    }
                }
            }
        }

        // Jeśli mamy do wysłania wybór
        if (choice_pending) {
            unsigned char choice = (unsigned char)button_clicked;
            if (send(sock, &choice, 1, 0) != 1) {
                perror("send");
                running = 0;
                break;
            }

            unsigned char response[2];
            ssize_t received = recv(sock, response, 2, 0);
            if (received == 2 && response[0] < 5 && response[1] <= 2) {
                printf("Partner wybrał: %s\n", button_labels[response[0]]);
                switch (response[1]) {
                    case 0:
                        printf("Wynik: różne wybory (mismatch).\n");
                        break;
                    case 1:
                        printf("Wynik: takie same wybory (accepted).\n");
                        break;
                    case 2:
                        printf("Wynik: oczekiwanie na drugiego gracza.\n");
                        break;
                    default:
                        printf("Nieznany status.\n");
                }
            } else {
                printf("Błąd odbioru od serwera lub nieprawidłowa odpowiedź.\n");
                running = 0;
                break;
            }
            choice_pending = 0; // odblokuj możliwość wyboru
        }

        // Rysowanie
        SDL_SetRenderDrawColor(ren, 200, 200, 200, 255);
        SDL_RenderClear(ren);

        for (int i = 0; i < 5; ++i) {
            if (choice_pending) {
                // Blokujemy kolor przycisków (np. szary)
                SDL_SetRenderDrawColor(ren, 150, 150, 150, 255);
            } else {
                SDL_SetRenderDrawColor(ren, 100, 100, 255, 255);
            }
            SDL_RenderFillRect(ren, &buttons[i]);
        }

        SDL_RenderPresent(ren);

        SDL_Delay(16); // około 60 fps
    }

    close(sock);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}
