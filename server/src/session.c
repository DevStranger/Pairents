#include "session.h"

static GameSession sessions[MAX_SESSIONS];

void init_sessions() {
    for (int i = 0; i < MAX_SESSIONS; ++i) {
        sessions[i].session_id = i;
        sessions[i].clients[0] = -1;
        sessions[i].clients[1] = -1;
        sessions[i].connected = 0;
    }
}

GameSession* assign_client_to_session(int client_fd) {
    for (int i = 0; i < MAX_SESSIONS; ++i) {
        if (sessions[i].connected < 2) {
            int pos = (sessions[i].clients[0] == -1) ? 0 : 1;
            sessions[i].clients[pos] = client_fd;
            sessions[i].connected++;
            return &sessions[i];
        }
    }
    return NULL;
}

GameSession* find_session_by_client(int client_fd) {
    for (int i = 0; i < MAX_SESSIONS; ++i) {
        if (sessions[i].clients[0] == client_fd || sessions[i].clients[1] == client_fd)
            return &sessions[i];
    }
    return NULL;
}
