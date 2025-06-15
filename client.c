#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
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

void draw_buttons(SDL_Renderer *ren, SDL_Rect *buttons) {
    SDL_SetRenderDrawColor(ren, 200, 200, 200, 255);
    SDL_RenderClear(ren);

    for (int i = 0; i < 5; ++i) {
        SDL_SetRenderDrawColor(ren, 100, 100, 255, 255);
        SDL_RenderFillRect(ren, &buttons[i]);
    }

    SDL_RenderPresent(ren);
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

    // Stałe połączenie z serwerem
    int sock = connect_to_server();
    if (sock < 0) {
        SDL_DestroyRenderer(ren);
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

    int running = 1;
    int waiting_for_response = 0;
    SDL_Event e;

    draw_buttons(ren, buttons);

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = 0;
            } else if (!waiting_for_response &&
                       e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int mx = e.button.x;
                int my = e.button.y;
                for (int i = 0; i < 5; ++i) {
                    if (mx >= buttons[i].x && mx <= buttons[i].x + buttons[i].w &&
                        my >= buttons[i].y && my <= buttons[i].y + buttons[i].h) {

                        unsigned char choice = (unsigned char)i;
                        if (send(sock, &choice, 1, 0) != 1) {
                            perror("send");
                            running = 0;
                            break;
                        }

                        waiting_for_response = 1;
                        break;
                    }
                }
            }
        }

        if (waiting_for_response) {
            unsigned char response[2];
            ssize_t received = recv(sock, response, 2, 0);
            if (received == 2) {
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
                waiting_for_response = 0;
                draw_buttons(ren, buttons);
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

        SDL_Delay(10);  // mały delay, żeby nie zjadało CPU
    }

    close(sock);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}
