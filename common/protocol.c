#include "protocol.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>

int send_msg(int sockfd, const char *json_str) {
	char msg[MAX_MSG_LEN];
	snprintf(msg, MAX_MSG_LEN, "%s\n", json_str);
	return send(sockfd, msg, strlen(msg), 0);
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
