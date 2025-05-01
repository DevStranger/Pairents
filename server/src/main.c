#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include "protocol.h"

#define PORT 12345
#define MAX_CLIENTS 100

int client_queue[MAX_CLIENTS];
int queue_len = 0;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *arg) {
    int sockfd = *(int *)arg;
    free(arg);

    printf("[THREAD] Client connected.\n");

    // dodawanie do kolejki matchmakingu
    pthread_mutex_lock(&queue_mutex);
    client_queue[queue_len++] = sockfd;

    if (queue_len >= 2) {
        int client1 = client_queue[0];
        int client2 = client_queue[1];

        // przesunięcie kolejki
        for (int i = 2; i < queue_len; i++) {
            client_queue[i - 2] = client_queue[i];
        }
        queue_len -= 2;

        pthread_mutex_unlock(&queue_mutex);

        printf("[MATCH] Pairing two clients!\n");

        send_msg(client1, "{\"status\":\"paired\"}");
        send_msg(client2, "{\"status\":\"paired\"}");

        close(client1);
        close(client2);
        printf("[MATCH] Pair closed.\n");

    } else {
        pthread_mutex_unlock(&queue_mutex);
        printf("[WAIT] Client is waiting for a partner...\n");
    }

    return NULL;
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 10);

    printf("[SERVER] Listening on port %d...\n", PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&addr, &addrlen);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        int *client_sock = malloc(sizeof(int));
        *client_sock = client_fd;
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, client_sock);
        pthread_detach(tid);  // nie trzeba joinować
    }

    close(server_fd);
    return 0;
}
