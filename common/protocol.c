#include "protocol.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

int send_msg(int sockfd, const char *json_str) {
    char msg[MAX_MSG_LEN];
    snprintf(msg, MAX_MSG_LEN, "%s\n", json_str);

    size_t len = strlen(msg);
    size_t total_sent = 0;

    while (total_sent < len) {
        ssize_t sent = send(sockfd, msg + total_sent, len - total_sent, 0);
        if (sent <= 0) {
            return -1; // błąd lub rozłączenie
        }
        total_sent += sent;
    }
    return total_sent;
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
