#ifndef PROTOCOL_H
#define PROTOCOL_H

#define MAX_MSG_LEN 1024

int send_msg(int sockfd, const char *json_str);
int recv_msg(int sockfd, char *buffer, int bufsize);

#endif
