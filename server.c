#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 12345
#define MAX_CLIENTS 10

typedef struct {
    int client1;
    int client2;
    int choice1;
    int choice2;
    int has_choice1;
    int has_choice2;
    pthread_mutex_t lock;
} Pair;

Pair pairs[MAX_CLIENTS / 2];
int pair_count = 0;
pthread_mutex_t pair_mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *arg) {
    int client_sock = *(int *)arg;
    free(arg);
    Pair *assigned_pair = NULL;
    int is_first;

    // Znajdź istniejącą parę lub utwórz nową
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
        pthread_mutex_init(&assigned_pair->lock, NULL);
        is_first = 1;
        pair_count++;
    }
    pthread_mutex_unlock(&pair_mutex);

    // Odbierz wybór klienta
    unsigned char button_choice;
    if (recv(client_sock, &button_choice, 1, 0) <= 0 || button_choice > 4) {
        perror("recv lub nieprawidłowy wybór");
        close(client_sock);
        return NULL;
    }

    // Zapisz wybór
   pthread_mutex_lock(&assigned_pair->lock);
    if (is_first) {
        assigned_pair->choice1 = button_choice;
        assigned_pair->has_choice1 = 1;
        printf("Client1 (sock %d) wybrał %d\n", client_sock, button_choice);
    } else {
        assigned_pair->choice2 = button_choice;
        assigned_pair->has_choice2 = 1;
        printf("Client2 (sock %d) wybrał %d\n", client_sock, button_choice);
    }
    pthread_mutex_unlock(&assigned_pair->lock);


   // Poczekaj aż obaj wybiorą
    while (1) {
        pthread_mutex_lock(&assigned_pair->lock);
        if (assigned_pair->has_choice1 && assigned_pair->has_choice2) {
            printf("Para ma wybory: choice1 = %d, choice2 = %d\n", assigned_pair->choice1, assigned_pair->choice2);
            unsigned char partner_choice = is_first ? assigned_pair->choice2 : assigned_pair->choice1;
            unsigned char status;
        
            if (assigned_pair->choice1 == assigned_pair->choice2) {
                status = 1; // accepted
            } else {
                status = 0; // mismatch
            }
        
            pthread_mutex_unlock(&assigned_pair->lock);
        
            unsigned char response[2] = {partner_choice, status};
            printf("Wysyłam klientowi [%d, %d]\n", response[0], response[1]);
            send(client_sock, response, 2, 0);
            break;
        }
        pthread_mutex_unlock(&assigned_pair->lock);
        usleep(10000);
    }

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
