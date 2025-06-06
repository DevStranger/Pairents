#ifndef PROTOCOL_H
#define PROTOCOL_H

#define MAX_MSG_LEN 1024

int send_msg(int sockfd, const char *json_str);
int recv_msg(int sockfd, char *buffer, int bufsize);

typedef enum {
    MSG_CONNECT,
    MSG_PAIR,
    MSG_ACTION,
    MSG_RESULT,
    MSG_DISCONNECT,
} MessageType;

typedef struct {
    MessageType type;
    int session_id;      // ID sesji gracza
    int player_id;       // np. 0 lub 1
    int action_code;     // kod akcji (np. kliknięcie guzika)
    char payload[256];   // dodatkowe dane
} Message;

#endif
