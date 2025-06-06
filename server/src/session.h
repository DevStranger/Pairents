#pragma once

#define MAX_SESSIONS 100

typedef struct {
    int session_id;
    int clients[2];       // sockety
    int connected;        // liczba połączonych
} GameSession;

void init_sessions();
GameSession* assign_client_to_session(int client_fd);
GameSession* find_session_by_client(int client_fd);
