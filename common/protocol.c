#include "protocol.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>

// Wysyła wiadomość JSON zakończoną znakiem nowej linii
int send_msg(int sockfd, const char *msg) {
    printf("[SEND] -> %d: %s\n", sockfd, msg);

    char buffer[1024];

    // Dodaj '\n' jako separator wiadomości
    int len = snprintf(buffer, sizeof(buffer), "%s\n", msg);
    if (len < 0 || len >= sizeof(buffer)) {
        fprintf(stderr, "send_msg: wiadomość za długa\n");
        return -1;
    }

    if (send(sockfd, buffer, len, 0) != len) {
        perror("send");
        return -1;
    }

    return 0;
}

// Odbiera jedną wiadomość (do znaku '\n') i zapisuje ją jako null-terminated string
int recv_msg(int sockfd, char *buffer, int bufsize) {
    int i = 0;
    char c;

    while (i < bufsize - 1) { // miejsce na '\0'
        int n = recv(sockfd, &c, 1, 0);
        if (n <= 0) {
            return -1;  // rozłączenie lub błąd
        }
        if (c == '\n') {
            break;  // koniec wiadomości
        }
        buffer[i++] = c;
    }

    buffer[i] = '\0';  // zakończ string
    return i;
}

// (opcjonalnie – do binarnej serializacji struktur)
void serialize_message(const Message* msg, char* buffer) {
    memcpy(buffer, msg, sizeof(Message));
}

void deserialize_message(const char* buffer, Message* msg) {
    memcpy(msg, buffer, sizeof(Message));
}
