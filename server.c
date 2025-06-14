#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 12345
#define MAX_CLIENTS 10

typedef struct {
    int client1;
    int client2;
    char char1;
    char char2;
    int has_char1;
    int has_char2;
} Pair;

Pair pairs[MAX_CLIENTS / 2];
int pair_count = 0;

void handle_client(int client_sock) {
    // Pairing logic
    Pair *assigned_pair = NULL;

    for (int i = 0; i < pair_count; ++i) {
        if (pairs[i].client2 == -1) {
            pairs[i].client2 = client_sock;
            assigned_pair = &pairs[i];
            break;
        }
    }

    if (!assigned_pair) {
        if (pair_count >= MAX_CLIENTS / 2) {
            printf("Too many clients!\n");
            close(client_sock);
            return;
        }
        pairs[pair_count].client1 = client_sock;
        pairs[pair_count].client2 = -1;
        pairs[pair_count].has_char1 = 0;
        pairs[pair_count].has_char2 = 0;
        assigned_pair = &pairs[pair_count];
        pair_count++;
    }

    char buffer;
    if (recv(client_sock, &buffer, 1, 0) <= 0) {
        perror("recv");
        close(client_sock);
        return;
    }

    // Store the character
    if (assigned_pair->client1 == client_sock) {
        assigned_pair->char1 = buffer;
        assigned_pair->has_char1 = 1;
    } else if (assigned_pair->client2 == client_sock) {
        assigned_pair->char2 = buffer;
        assigned_pair->has_char2 = 1;
    }

    // Wait for the other client's input
    while (!(assigned_pair->has_char1 && assigned_pair->has_char2)) {
        usleep(1000);
    }

    // Send back the partner's character
    char partner_char = (assigned_pair->client1 == client_sock) ? assigned_pair->char2 : assigned_pair->char1;
    send(client_sock, &partner_char, 1, 0);

    close(client_sock);
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(PORT)
    };

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, MAX_CLIENTS);
    printf("Server is listening on port %d...\n", PORT);

    while (1) {
        int addrlen = sizeof(address);
        int client_sock = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (client_sock < 0) {
            perror("accept");
            continue;
        }
        if (fork() == 0) {
            // Child process
            close(server_fd);
            handle_client(client_sock);
            exit(0);
        }
        close(client_sock); // Parent doesn't need this
    }

    return 0;
}
