#include "protocol.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdbool.h>

int send_msg(int sockfd, const char *msg) {
    printf("[SEND] -> %d: %s\n", sockfd, msg);

    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%s\n", msg); 

    int len = strlen(buffer);
    if (send(sockfd, buffer, len, 0) != len) {
        perror("send");
        return -1;
    }
    return 0;
}

int recv_msg(int sockfd, char *buffer, int bufsize) {
	int i = 0;
	char c;
	while (i < bufsize -1) {
		int n = recv(sockfd, &c, 1, 0);
		if (n <= 0) break;
		if (c == '\n') break;
		buffer[i++] = c;
	}
	buffer[i] = '\0';
	return i;
}

void serialize_message(const Message* msg, char* buffer) {
    memcpy(buffer, msg, sizeof(Message));
}

void deserialize_message(const char* buffer, Message* msg) {
    memcpy(msg, buffer, sizeof(Message));
}
