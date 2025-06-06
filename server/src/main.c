// server/main.c

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

// --- Struktura sesji gry ---
typedef struct {
    int sock1, sock2;           // sockety dwóch graczy
    char game_id[32];           // unikalny ID sesji
    char action1[32];           // akcja gracza 1
    char action2[32];           // akcja gracza 2
    bool resolved;              // czy akcje są zsynchronizowane
    time_t window_start;        // czas rozpoczęcia okna synchronizacji
} GameSession;

static GameSession sessions[MAX_SESSIONS];
static int session_count = 0;

static int client_queue[MAX_CLIENTS];
static int queue_len = 0;

static pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

// Funkcja nadaje unikalny game_id dla sesji na podstawie indeksu
void assign_game_id(char *buffer, int index) {
    snprintf(buffer, 32, "PAIR_%03d", index);
}

// --- Odbieranie akcji od klienta i synchronizacja ---
void *client_listener(void *arg) {
    int sockfd = *(int *)arg;
    free(arg);

    char buffer[1024];
    while (1) {
        int len = recv_msg(sockfd, buffer, sizeof(buffer));
        if (len <= 0) {
            printf("[DISCONNECT] Client disconnected (sock=%d)\n", sockfd);
            close(sockfd);
            break;
        }

        cJSON *json = cJSON_Parse(buffer);
        if (!json) {
            fprintf(stderr, "[ERROR] JSON parse error from client %d\n", sockfd);
            continue;
        }

        // Pobieramy game_id i action z JSON
        const cJSON *game_id_json = cJSON_GetObjectItemCaseSensitive(json, "game_id");
        const cJSON *action_json = cJSON_GetObjectItemCaseSensitive(json, "action");

        if (!cJSON_IsString(game_id_json) || !cJSON_IsString(action_json)) {
            cJSON_Delete(json);
            continue;
        }

        const char *gid = game_id_json->valuestring;
        const char *action = action_json->valuestring;

        // Znajdujemy sesję po game_id
        pthread_mutex_lock(&queue_mutex);
        for (int i = 0; i < session_count; i++) {
            if (strcmp(sessions[i].game_id, gid) == 0) {
                if (sockfd == sessions[i].sock1) {
                    strcpy(sessions[i].action1, action);
                } else if (sockfd == sessions[i].sock2) {
                    strcpy(sessions[i].action2, action);
                }

                // Sprawdzamy, czy obie akcje są przesłane i czy się zgadzają
                if (!sessions[i].resolved &&
                    strlen(sessions[i].action1) > 0 &&
                    strlen(sessions[i].action2) > 0) {

                    if (strcmp(sessions[i].action1, sessions[i].action2) == 0) {
                        sessions[i].resolved = true;

                        char response[128];
                        snprintf(response, sizeof(response),
                            "{\"status\":\"accepted\",\"action\":\"%s\"}",
                            sessions[i].action1);
                        send_msg(sessions[i].sock1, response);
                        send_msg(sessions[i].sock2, response);
                        printf("[SYNC] Action '%s' confirmed by both players in session %s.\n",
                               sessions[i].action1, gid);
                    } else {
                        send_msg(sessions[i].sock1, "{\"status\":\"mismatch\"}");
                        send_msg(sessions[i].sock2, "{\"status\":\"mismatch\"}");
                        printf("[SYNC] Action mismatch in session %s.\n", gid);
                    }

                    // Możesz też tu wyczyścić akcje na kolejną rundę:
                    // strcpy(sessions[i].action1, "");
                    // strcpy(sessions[i].action2, "");
                    // sessions[i].resolved = false;
                }
                break;
            }
        }
        pthread_mutex_unlock(&queue_mutex);

        cJSON_Delete(json);
    }

    return NULL;
}

// --- Obsługa nowego klienta, parowanie ---
void *handle_client(void *arg) {
    int sockfd = *(int *)arg;
    free(arg);

    printf("[CONNECT] Client connected: sock=%d\n", sockfd);

    pthread_mutex_lock(&queue_mutex);

    // Dodaj klienta do kolejki
    client_queue[queue_len++] = sockfd;

    if (queue_len >= 2) {
        int client1 = client_queue[0];
        int client2 = client_queue[1];

        // Usuwamy pierwsze 2 z kolejki
        memmove(client_queue, client_queue + 2, sizeof(int) * (queue_len - 2));
        queue_len -= 2;

        // Tworzymy nową sesję
        if (session_count >= MAX_SESSIONS) {
            fprintf(stderr, "[ERROR] Max sessions reached\n");
            pthread_mutex_unlock(&queue_mutex);
            close(client1);
            close(client2);
            return NULL;
        }

        assign_game_id(sessions[session_count].game_id, session_count);
        sessions[session_count].sock1 = client1;
        sessions[session_count].sock2 = client2;
        sessions[session_count].action1[0] = '\0';
        sessions[session_count].action2[0] = '\0';
        sessions[session_count].resolved = false;
        sessions[session_count].window_start = time(NULL);

        // Powiadamiamy graczy o sparowaniu
        char game_msg[128];
        snprintf(game_msg, sizeof(game_msg),
                 "{\"status\":\"paired\",\"game_id\":\"%s\"}",
                 sessions[session_count].game_id);
        send_msg(client1, game_msg);
        send_msg(client2, game_msg);

        // Uruchamiamy listenerów dla obu klientów sesji
        int *c1 = malloc(sizeof(int));
        int *c2 = malloc(sizeof(int));
        *c1 = client1;
        *c2 = client2;

        pthread_t tid1, tid2;
        pthread_create(&tid1, NULL, client_listener, c1);
        pthread_create(&tid2, NULL, client_listener, c2);
        pthread_detach(tid1);
        pthread_detach(tid2);

        printf("[PAIR] New game session created: %s (clients %d, %d)\n",
               sessions[session_count].game_id, client1, client2);

        session_count++;
    }

    pthread_mutex_unlock(&queue_mutex);

    return NULL;
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    // Tworzymy socket serwera
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("[SERVER] Listening on port %d...\n", PORT);

    // Główna pętla akceptująca klientów
    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&addr, &addrlen);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        // Obsługa klienta w osobnym wątku
        int *client_sock = malloc(sizeof(int));
        *client_sock = client_fd;

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, client_sock) != 0) {
            perror("pthread_create");
            close(client_fd);
            free(client_sock);
            continue;
        }
        pthread_detach(tid);
    }

    close(server_fd);
    return 0;
}
