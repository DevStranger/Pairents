#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#define PORT 12345
#define SERVER_IP "127.0.0.1"

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in serv_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT)
    };

    inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    printf("Wpisz znak: ");
    char c;
    scanf(" %c", &c);
    send(sock, &c, 1, 0);

    char received;
    if (recv(sock, &received, 1, 0) > 0) {
        printf("Partner wysłał: %c\n", received);
    } else {
        printf("Błąd odbioru od serwera.\n");
    }

    close(sock);
    return 0;
}
