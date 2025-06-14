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
    char char1;
    char char2;
    int has_char1;
    int has_char2;
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

    // Szukamy pary lub tworzymy nową
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
            printf("Too many clients!\n");
            close(client_sock);
            pthread_mutex_unlock(&pair_mutex);
            return NULL;
        }
        assigned_pair = &pairs[pair_count];
        assigned_pair->client1 = client_sock;
        assigned_pair->client2 = -1;
        assigned_pair->has_char1 = 0;
        assigned_pair->has_char2 = 0;
        pthread_mutex_init(&assigned_pair->lock, NULL);
        is_first = 1;
        pair_count++;
    }
    pthread_mutex_unlock(&pair_mutex);

    // Odbierz znak
    char buffer;
    if (recv(client_sock, &buffer, 1, 0) <= 0) {
        perror("recv");
        close(client_sock);
        return NULL;
    }

    // Zapisz znak w parze
    pthread_mutex_lock(&assigned_pair->lock);
    if (is_first) {
        assigned_pair->char1 = buffer;
        assigned_pair->has_char1 = 1;
    } else {
        assigned_pair->char2 = buffer;
        assigned_pair->has_char2 = 1;
    }
    pthread_mutex_unlock(&assigned_pair->lock);

    // Czekaj aż obaj wyślą znaki
    while (1) {
        pthread_mutex_lock(&assigned_pair->lock);
        if (assigned_pair->has_char1 && assigned_pair->has_char2) {
            char to_send = is_first ? assigned_pair->char2 : assigned_pair->char1;
            pthread_mutex_unlock(&assigned_pair->lock);
            send(client_sock, &to_send, 1, 0);
            break;
        }
        pthread_mutex_unlock(&assigned_pair->lock);
        usleep(10000); // odciążenie CPU
    }

    close(client_sock);
    return NULL;
}

int main() {
    // Inicjalizacja par
    for (int i = 0; i < MAX_CLIENTS / 2; i++) {
        pairs[i].client1 = -1;
        pairs[i].client2 = -1;
        pairs[i].has_char1 = 0;
        pairs[i].has_char2 = 0;
        pthread_mutex_init(&pairs[i].lock, NULL);
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Serwer nasłuchuje na porcie %d\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int *client_sock = malloc(sizeof(int));
        if (!client_sock) {
            perror("malloc");
            continue;
        }

        *client_sock = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
        if (*client_sock < 0) {
            perror("accept");
            free(client_sock);
            continue;
        }

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, client_sock) != 0) {
            perror("pthread_create");
            close(*client_sock);
            free(client_sock);
            continue;
        }
        pthread_detach(tid);
    }

    return 0;
}
