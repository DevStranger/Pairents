#ifndef PROTOCOL_H
#define PROTOCOL_H

#define MSG_MAX 1024

typedef enum { MSG_PING, MSG_PONG } msg_type_t;
typedef struct {
	msg_type_t type;
	char payload[MSG_MAX];
} message_t;

#endif
