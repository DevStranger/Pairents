#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include "protocol.h"
#include "cJSON.h"

#define PORT 12345
#define MAX_CLIENTS 100
#define MAX_SESSIONS 50

// --- Struktura gry ---
typedef struct {
    int sock1, sock2;
    char game_id[32];
    char action1[32];
    char action2[32];
    bool resolved;
    time_t window_start;
} GameSession;

int client_queue[MAX_CLIENTS];
int queue_len = 0;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

GameSession sessions[MAX_SESSIONS];
int session_count = 0;

void assign_game_id(char *buffer, int index) {
    sprintf(buffer, "PAIR_%03d", index);
}

// --- Odbiera akcje od klientów ---
void *client_listener(void *arg) {
    int sockfd = *(int *)arg;
    char buffer[1024];

    while (1) {
        int len = recv_msg(sockfd, buffer, sizeof(buffer));
        if (len <= 0) {
            close(sockfd);
            break;
        }

        cJSON *json = cJSON_Parse(buffer);
        if (!json) continue;

        const cJSON *game_id_json = cJSON_GetObjectItemCaseSensitive(json, "game_id");
        const cJSON *action_json = cJSON_GetObjectItemCaseSensitive(json, "action");
        if (!cJSON_IsString(game_id_json) || !cJSON_IsString(action_json)) {
            cJSON_Delete(json);
            continue;
        }

        const char *gid = game_id_json->valuestring;
        const char *action = action_json->valuestring;

        for (int i = 0; i < session_count; i++) {
            if (strcmp(sessions[i].game_id, gid) == 0) {
                if (sockfd == sessions[i].sock1) {
                    strcpy(sessions[i].action1, action);
                } else if (sockfd == sessions[i].sock2) {
                    strcpy(sessions[i].action2, action);
                }

                if (!sessions[i].resolved &&
                    strlen(sessions[i].action1) > 0 &&
                    strlen(sessions[i].action2) > 0) {

                    if (strcmp(sessions[i].action1, sessions[i].action2) == 0) {
                        sessions[i].resolved = true;

                        char response[128];
                        sprintf(response, "{\"status\":\"accepted\",\"action\":\"%s\"}", sessions[i].action1);
                        send_msg(sessions[i].sock1, response);
                        send_msg(sessions[i].sock2, response);
                        printf("[SYNC] Action %s confirmed by both players.\n", sessions[i].action1);
                    } else {
                        send_msg(sessions[i].sock1, "{\"status\":\"mismatch\"}");
                        send_msg(sessions[i].sock2, "{\"status\":\"mismatch\"}");
                        printf("[SYNC] Action mismatch.\n");
                    }
                }
                break;
            }
        }

        cJSON_Delete(json);
    }

    return NULL;
}

// --- Główna obsługa klienta: tworzenie pary ---
void *handle_client(void *arg) {
    int sockfd = *(int *)arg;
    free(arg);
    printf("[THREAD] Client connected.\n");

    pthread_mutex_lock(&queue_mutex);
    client_queue[queue_len++] = sockfd;

    if (queue_len >= 2) {
        int client1 = client_queue[0];
        int client2 = client_queue[1];

        for (int i = 2; i < queue_len; i++) {
            client_queue[i - 2] = client_queue[i];
        }
        queue_len -= 2;

        assign_game_id(sessions[session_count].game_id, session_count);
        sessions[session_count].sock1 = client1;
        sessions[session_count].sock2 = client2;
        strcpy(sessions[session_count].action1, "");
        strcpy(sessions[session_count].action2, "");
        sessions[session_count].resolved = false;
        sessions[session_count].window_start = time(NULL);

        char game_msg[128];
        sprintf(game_msg, "{\"status\":\"paired\",\"game_id\":\"%s\"}", sessions[session_count].game_id);
        send_msg(client1, game_msg);
        send_msg(client2, game_msg);

        int *c1 = malloc(sizeof(int));
        int *c2 = malloc(sizeof(int));
        *c1 = client1;
        *c2 = client2;

        pthread_t tid1, tid2;
        pthread_create(&tid1, NULL, client_listener, c1);
        pthread_create(&tid2, NULL, client_listener, c2);
        pthread_detach(tid1);
        pthread_detach(tid2);

        session_count++;
    }

    pthread_mutex_unlock(&queue_mutex);
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
        pthread_detach(tid);
    }

    close(server_fd);
    return 0;
}

