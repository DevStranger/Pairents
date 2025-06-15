#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 12345
#define MAX_CLIENTS 10

typedef struct {
    unsigned char hunger;
    unsigned char happiness;
    unsigned char sleep;
    unsigned char health;
    unsigned char growth;
    unsigned char love;
} Creature;

typedef struct {
    int client1;
    int client2;
    int choice1;
    int choice2;
    int has_choice1;
    int has_choice2;
    Creature creature;
    pthread_mutex_t lock;
} Pair;

Pair pairs[MAX_CLIENTS / 2];
int pair_count = 0;
pthread_mutex_t pair_mutex = PTHREAD_MUTEX_INITIALIZER;

void apply_action(Creature *c, unsigned char action) {
    // Przykładowa prosta aktualizacja stanu stworzenia w zależności od akcji
    switch (action) {
        case 0: // Feed
            c->hunger = (c->hunger > 5) ? c->hunger - 5 : 0;
            c->happiness = (c->happiness + 2 < 100) ? c->happiness + 2 : 100;
            break;
        case 1: // Read
            c->happiness = (c->happiness + 3 < 100) ? c->happiness + 3 : 100;
            c->growth = (c->growth + 1 < 100) ? c->growth + 1 : 100;
            break;
        case 2: // Sleep
            c->sleep = (c->sleep + 5 < 100) ? c->sleep + 5 : 100;
            c->health = (c->health + 2 < 100) ? c->health + 2 : 100;
            break;
        case 3: // Hug
            c->love = (c->love + 4 < 100) ? c->love + 4 : 100;
            c->happiness = (c->happiness + 3 < 100) ? c->happiness + 3 : 100;
            break;
        case 4: // Play
            c->happiness = (c->happiness + 5 < 100) ? c->happiness + 5 : 100;
            c->growth = (c->growth + 2 < 100) ? c->growth + 2 : 100;
            break;
        default:
            break;
    }
}

void send_response(int sock, unsigned char partner_choice, unsigned char status) {
    unsigned char response[2] = {partner_choice, status};
    send(sock, response, 2, 0);
}

void *handle_client(void *arg) {
    int client_sock = *(int *)arg;
    free(arg);
    Pair *assigned_pair = NULL;
    int is_first;

    // Przypisz klienta do pary
    pthread_mutex_lock(&pair_mutex);
    for (int i = 0; i < pair_count; ++i) {
        if (pairs[i].client2 == -1) {
            pairs[i].client2 = client_sock;
            assigned_pair = &pairs[i];
            is_first = 0;
            break;
        }
    }
    if (!assigned_pair) {
        if (pair_count >= MAX_CLIENTS / 2) {
            printf("Zbyt wielu klientów!\n");
            close(client_sock);
            pthread_mutex_unlock(&pair_mutex);
            return NULL;
        }
        assigned_pair = &pairs[pair_count];
        assigned_pair->client1 = client_sock;
        assigned_pair->client2 = -1;
        assigned_pair->has_choice1 = 0;
        assigned_pair->has_choice2 = 0;
        memset(&assigned_pair->creature, 0, sizeof(Creature));
        pthread_mutex_init(&assigned_pair->lock, NULL);
        is_first = 1;
        pair_count++;
    }
    pthread_mutex_unlock(&pair_mutex);

    while (1) {
        unsigned char button_choice;
        ssize_t rcv = recv(client_sock, &button_choice, 1, 0);
        if (rcv <= 0) {
            printf("Klient rozłączył się lub błąd recv\n");
            break;
        }

        if (button_choice > 4) continue;

        pthread_mutex_lock(&assigned_pair->lock);

        // Sprawdź, czy klient nie wybrał już akcji w tej turze
        if (is_first && assigned_pair->has_choice1) {
            pthread_mutex_unlock(&assigned_pair->lock);
            continue;
        }
        if (!is_first && assigned_pair->has_choice2) {
            pthread_mutex_unlock(&assigned_pair->lock);
            continue;
        }

        if (is_first) {
            assigned_pair->choice1 = button_choice;
            assigned_pair->has_choice1 = 1;
        } else {
            assigned_pair->choice2 = button_choice;
            assigned_pair->has_choice2 = 1;
        }

        // Sprawdź, czy obaj gracze wybrali już akcję
        if (assigned_pair->has_choice1 && assigned_pair->has_choice2) {
            unsigned char status = (assigned_pair->choice1 == assigned_pair->choice2) ? 1 : 0;
            unsigned char c1 = assigned_pair->choice1;
            unsigned char c2 = assigned_pair->choice2;

            if (status == 1) {
                apply_action(&assigned_pair->creature, assigned_pair->choice1);
            }

            int sock1 = assigned_pair->client1;
            int sock2 = assigned_pair->client2;

            pthread_mutex_unlock(&assigned_pair->lock);

            // Wyślij status i wybór przeciwnika
            if (sock1 != -1) send_response(sock1, c2, status);
            if (sock2 != -1) send_response(sock2, c1, status);

            // Jeśli accepted, wyślij aktualny stan stworzenia
            if (status == 1) {
                if (sock1 != -1) send(sock1, &assigned_pair->creature, sizeof(Creature), 0);
                if (sock2 != -1) send(sock2, &assigned_pair->creature, sizeof(Creature), 0);
            }

            pthread_mutex_lock(&assigned_pair->lock);
            assigned_pair->has_choice1 = 0;
            assigned_pair->has_choice2 = 0;
            pthread_mutex_unlock(&assigned_pair->lock);
        } else {
            // Jeden gracz już wybrał, drugi musi czekać
            pthread_mutex_unlock(&assigned_pair->lock);
            send_response(client_sock, 0, 2); // status 2 = oczekiwanie
        }
    }

    // Obsługa rozłączenia klienta
    pthread_mutex_lock(&pair_mutex);
    pthread_mutex_lock(&assigned_pair->lock);

    if (is_first) {
        assigned_pair->client1 = -1;
        assigned_pair->has_choice1 = 0;
    } else {
        assigned_pair->client2 = -1;
        assigned_pair->has_choice2 = 0;
    }

    // Jeśli obaj gracze odeszli, "wyczyść" parę, zmniejsz pair_count jeśli to ostatnia para
    if (assigned_pair->client1 == -1 && assigned_pair->client2 == -1) {
        // Wyczyść dane
        memset(&assigned_pair->creature, 0, sizeof(Creature));
        assigned_pair->choice1 = 0;
        assigned_pair->choice2 = 0;
        assigned_pair->has_choice1 = 0;
        assigned_pair->has_choice2 = 0;
        // opcjonalnie można "usunąć" parę z tablicy i przesunąć pozostałe, ale to komplikuje
        // dla prostoty pozostawiamy tak
    }

    pthread_mutex_unlock(&assigned_pair->lock);
    pthread_mutex_unlock(&pair_mutex);

    close(client_sock);
    return NULL;
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Serwer nasłuchuje na porcie %d...\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int *client_sock = malloc(sizeof(int));
        if (!client_sock) continue;

        *client_sock = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
        if (*client_sock < 0) {
            perror("accept");
            free(client_sock);
            continue;
        }

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, client_sock);
        pthread_detach(tid);
    }

    close(server_fd);
    return 0;
}
