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

typedef struct {
    int sock1, sock2;
    char game_id[32];
    char action1[32];
    char action2[32];
    bool resolved;
    bool action1_ready;
    bool action2_ready;
    time_t window_start;
} GameSession;

static GameSession sessions[MAX_SESSIONS];
static int session_count = 0;

static int client_queue[MAX_CLIENTS];
static int queue_len = 0;

static pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

void assign_game_id(char *buffer, int index) {
    snprintf(buffer, 32, "PAIR_%03d", index);
}

void *client_listener(void *arg) {
    int sockfd = *(int *)arg;
    printf("[DEBUG] Listener active for sock=%d\n", sockfd);
    free(arg);

    char buffer[1024];

    while (1) {
        int len = recv_msg(sockfd, buffer, sizeof(buffer));
        if (len <= 0) {
            printf("[DISCONNECT] Client disconnected (sock=%d)\n", sockfd);
            close(sockfd);
            return NULL;
        }

        cJSON *json = cJSON_Parse(buffer);
        if (!json) {
            fprintf(stderr, "[ERROR] JSON parse error from client %d\n", sockfd);
            continue;
        }

        const cJSON *game_id_json = cJSON_GetObjectItemCaseSensitive(json, "game_id");
        const cJSON *action_json  = cJSON_GetObjectItemCaseSensitive(json, "action");

        if (!cJSON_IsString(game_id_json) || !cJSON_IsString(action_json)) {
            cJSON_Delete(json);
            continue;
        }

        const char *gid = game_id_json->valuestring;
        const char *action = action_json->valuestring;

        pthread_mutex_lock(&queue_mutex);

        bool session_found = false;

        for (int i = 0; i < session_count; i++) {
            GameSession *s = &sessions[i];
            if (strcmp(s->game_id, gid) != 0) continue;

            session_found = true;

            if (sockfd == s->sock1) {
                strncpy(s->action1, action, sizeof(s->action1) - 1);
                s->action1[sizeof(s->action1) - 1] = '\0';
                printf("[INFO] Session %d: Received action1='%s' from sock=%d\n", i, s->action1, sockfd);
            } else if (sockfd == s->sock2) {
                strncpy(s->action2, action, sizeof(s->action2) - 1);
                s->action2[sizeof(s->action2) - 1] = '\0';
                printf("[INFO] Session %d: Received action2='%s' from sock=%d\n", i, s->action2, sockfd);
            }

            // Wysyłamy dokładnie to co było w oryginale (bez zmian)
            char reply[256];
            snprintf(reply, sizeof(reply),
                     "{\"type\":%d,\"session_id\":%d,\"status\":\"wait\",\"payload\":\"\"}",
                     MSG_RESULT, i);
            send_msg(sockfd, reply);

            break;
        }

        pthread_mutex_unlock(&queue_mutex);

        if (!session_found) {
            printf("[WARN] Session not found for game_id=%s sock=%d\n", gid, sockfd);
        }

        cJSON_Delete(json);
    }

    return NULL;
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

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

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&addr, &addrlen);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

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
