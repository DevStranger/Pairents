#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "creature.h"  

#define PORT 12345
#define MAX_CLIENTS 10

// para graczy
typedef struct {
    int client1;
    int client2;
    int choice1;
    int choice2;
    int has_choice1;
    int has_choice2;
    Creature creature;       
    pthread_mutex_t lock;
    char id1[INET_ADDRSTRLEN];  // IP pierwszego gracza
    char id2[INET_ADDRSTRLEN];  // IP drugiego gracza
} Pair;

// informacje o pojedynczym kliencie
typedef struct {
    int sock;
    char ip[INET_ADDRSTRLEN];
} ClientInfo;

Pair pairs[MAX_CLIENTS / 2];                                // tablica par
int pair_count = 0;                                         // ilość par
pthread_mutex_t pair_mutex = PTHREAD_MUTEX_INITIALIZER;     // mutex do ochrony tablicy par

// funkcja zapisująca akktualny stan par do pliku binarnego
void save_pairs_to_file(const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("save_pairs_to_file");
        return;
    }
    fwrite(&pair_count, sizeof(int), 1, fp);
    for (int i = 0; i < pair_count; i++) {
        fwrite(&pairs[i], sizeof(Pair), 1, fp);
    }
    fclose(fp);
}

// funkcja do wczytywania stanu par z pliku binarnego
void load_pairs_from_file(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) return;

    fread(&pair_count, sizeof(int), 1, fp);
    for (int i = 0; i < pair_count; i++) {
        fread(&pairs[i], sizeof(Pair), 1, fp);
        pthread_mutex_init(&pairs[i].lock, NULL);  // odtwarzamy mutex
    }
    fclose(fp);
}

// funkcja wysyłania odpowiedzi do klienta
// partner_choice = wybrana akcja partnera
// status = 0 = mismatch, 1 = accepted, 2 = wait
void send_response(int sock, unsigned char partner_choice, unsigned char status) {
    unsigned char response[2] = {partner_choice, status};
    send(sock, response, 2, 0);
    printf("[=>] Wysłano do %d: partner_choice=%d, status=%d\n", sock, partner_choice, status);
}

// funkcja zamieniająca numer akcji na czytelną nazwę (używana do logów)
const char* get_action_name(unsigned char action) {
    switch (action) {
        case 0: return "Feed";
        case 1: return "Read";
        case 2: return "Sleep";
        case 3: return "Hug";
        case 4: return "Play";
        default: return "Unknown";
    }
}

// funkcja obsługująca pojedynczego klienta w osobnym wątku
void *handle_client(void *arg) {
    ClientInfo *client_info = (ClientInfo *)arg;
    int client_sock = client_info->sock;
    char *client_ip = client_info->ip;
    free(client_info);
    
    Pair *assigned_pair = NULL;
    int is_first = -1; // 1 = client1, 0 = client2

    pthread_mutex_lock(&pair_mutex);

    // 1. Sprawdzamy, czy klient z tym IP już jest w parach i nie jest podłączony (reconnect)
    for (int i = 0; i < pair_count; i++) {
        if (strcmp(pairs[i].id1, client_ip) == 0 && pairs[i].client1 == -1) {
            pairs[i].client1 = client_sock;
            assigned_pair = &pairs[i];
            is_first = 1;
            break;
        }
        if (strcmp(pairs[i].id2, client_ip) == 0 && pairs[i].client2 == -1) {
            pairs[i].client2 = client_sock;
            assigned_pair = &pairs[i];
            is_first = 0;
            break;
        }
    }

    // 2. Jeśli klient nie jest w parach, próbujemy go przypisać do pary z wolnym client2
    if (!assigned_pair) {
        for (int i = 0; i < pair_count; i++) {
            if (pairs[i].client2 == -1) {
                pairs[i].client2 = client_sock;
                assigned_pair = &pairs[i];
                is_first = 0;
                break;
            }
        }
    }

    // 3. Jeśli dalej nie przypisany, tworzymy nową parę
    if (!assigned_pair) {
        if (pair_count >= MAX_CLIENTS / 2) {
            printf("[!] Zbyt wielu klientów! Rozłączam %d\n", client_sock);
            close(client_sock);
            pthread_mutex_unlock(&pair_mutex);
            return NULL;
        }
        assigned_pair = &pairs[pair_count];
        assigned_pair->client1 = client_sock;
        assigned_pair->client2 = -1;
        assigned_pair->has_choice1 = 0;
        assigned_pair->has_choice2 = 0;
        pthread_mutex_init(&assigned_pair->lock, NULL);
        init_creature(&assigned_pair->creature);   // inicjalizacja stwora
        is_first = 1;
        pair_count++;
    }

    // zapis IP jeśli nowy klient w parze
    if (is_first == 1) {
        strcpy(assigned_pair->id1, client_ip);
    } else if (is_first == 0) {
        strcpy(assigned_pair->id2, client_ip);
    }

    pthread_mutex_unlock(&pair_mutex);

    if (is_first == 1) {
        printf("[*] Klient %d (%s) przypisany jako PIERWSZY w parze #%ld\n", client_sock, assigned_pair->id1, assigned_pair - pairs);
    } else {
        printf("[*] Klient %d (%s) przypisany jako DRUGI w parze #%ld\n", client_sock, assigned_pair->id2, assigned_pair - pairs);
    }

    // główna pętla odbierająca wybory klienta i synchronizująca z partnerem
    while (1) {
        unsigned char button_choice;
        ssize_t rcv = recv(client_sock, &button_choice, 1, 0);
        if (rcv <= 0) {
            printf("[-] Klient %d się rozłączył lub błąd recv\n", client_sock);
            break;
        }

        printf("[>] Klient %d wybrał akcję: %s (%d)\n", client_sock, get_action_name(button_choice), button_choice);
        if (button_choice > 4) continue;

        pthread_mutex_lock(&assigned_pair->lock);

        // zapis wyboru klienta, jeśli jeszcze nie wybierał w ramach tej "tury"
        if (is_first == 1) {
            if (assigned_pair->has_choice1) {
                pthread_mutex_unlock(&assigned_pair->lock);
                continue; // już wybrał w tej turze
            }
            assigned_pair->choice1 = button_choice;
            assigned_pair->has_choice1 = 1;
        } else {
            if (assigned_pair->has_choice2) {
                pthread_mutex_unlock(&assigned_pair->lock);
                continue;
            }
            assigned_pair->choice2 = button_choice;
            assigned_pair->has_choice2 = 1;
        }

        // jeśli oboje wybrali już akcję, sprawdzamy zgodność i odpowiednio aktualizujemy stwora
        if (assigned_pair->has_choice1 && assigned_pair->has_choice2) {
            unsigned char status = (assigned_pair->choice1 == assigned_pair->choice2) ? 1 : 0;
            unsigned char c1 = assigned_pair->choice1;
            unsigned char c2 = assigned_pair->choice2;

            printf("[✓] Para #%ld: gracz1=%s (%d), gracz2=%s (%d) -> %s\n",
                   assigned_pair - pairs,
                   get_action_name(c1), c1,
                   get_action_name(c2), c2,
                   status == 1 ? "ZGODNE" : "NIEZGODNE");

            if (status == 1) {
                // jeśli wybrali to samo, wykonujemy akcję i aktualizujemy stwora
                apply_action(&assigned_pair->creature, c1);
                update_creature(&assigned_pair->creature);
                save_pairs_to_file("pairs.dat");
                printf("[⇄] Aktualizacja stanu stwora w parze #%ld (akcja: %s)\n", assigned_pair - pairs, get_action_name(c1));
            }

            int sock1 = assigned_pair->client1;
            int sock2 = assigned_pair->client2;

            pthread_mutex_unlock(&assigned_pair->lock);

            // wysyłamy odpowiedź do obu klientów z akcją partnera i wynikiem porównania
            if (sock1 != -1) send_response(sock1, c2, status);
            if (sock2 != -1) send_response(sock2, c1, status);

            // jeśli wybrali to samo, wysyłamy też stan stwora po aktualizacji
            if (status == 1) {
                if (sock1 != -1) send(sock1, &assigned_pair->creature, sizeof(Creature), 0);
                if (sock2 != -1) send(sock2, &assigned_pair->creature, sizeof(Creature), 0);
            }

            // reset na kolejną turę
            pthread_mutex_lock(&assigned_pair->lock);
            assigned_pair->has_choice1 = 0;
            assigned_pair->has_choice2 = 0;
            pthread_mutex_unlock(&assigned_pair->lock);
        } else {
            // jeśli tylko jeden gracz w parze wybrał, czekamy na drugiego
            unsigned char partner_choice = 0;
            unsigned char status = 2; 
            printf("[~] Klient %d czeka na drugiego gracza...\n", client_sock);
            pthread_mutex_unlock(&assigned_pair->lock);
            send_response(client_sock, partner_choice, status);
        }
    }

    // zamykamy połączenie i ustawiamy socket w parze na -1 (client się rozłączył)
    pthread_mutex_lock(&pair_mutex);
    if (is_first == 1) {
        assigned_pair->client1 = -1;
        printf("[*] Klient %d (%s) rozłączył się (client1)\n", client_sock, client_ip);
    } else {
        assigned_pair->client2 = -1;
        printf("[*] Klient %d (%s) rozłączył się (client2)\n", client_sock, client_ip);
    }
    pthread_mutex_unlock(&pair_mutex);

    close(client_sock);
    return NULL;
}

// czasowe update-y
void *periodic_update_thread(void *arg) {
    while (1) {
        pthread_mutex_lock(&pair_mutex);
        for (int i = 0; i < pair_count; i++) {
            pthread_mutex_lock(&pairs[i].lock);
            update_creature(&pairs[i].creature);
            // wysyłamy stan stwora do obu klientów, jeśli są połączeni
            if (pairs[i].client1 != -1)
                send(pairs[i].client1, &pairs[i].creature, sizeof(Creature), 0);
            if (pairs[i].client2 != -1)
                send(pairs[i].client2, &pairs[i].creature, sizeof(Creature), 0);
            pthread_mutex_unlock(&pairs[i].lock);
        }
        pthread_mutex_unlock(&pair_mutex);

        sleep(60);  // co 60 sekund
    }
    return NULL;
}

int main() {
    // tworzymy gniazdo serwera TCP
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // ustawienie opcji SO_REUSEADDR aby umożliwić szybkie ponowne uruchomienie serwera
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // konfiguracja adresu serwera
    struct sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    // bindowanie gniazda do adresu i portu
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // wczytanie wcześniej zapisanych par klientów (jeśli istnieją)
    load_pairs_from_file("pairs.dat");

    // ustawienie serwera w tryb nasłuchiwania
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Serwer nasłuchuje na porcie %d...\n", PORT);

    // update-y
    pthread_t updater_tid;
    pthread_create(&updater_tid, NULL, periodic_update_thread, NULL);
    pthread_detach(updater_tid);

    // główna pętla akceptująca połączenia od klientów
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        
        // alokujemy pamięć na dane klienta, aby przekazać je do wątku
        ClientInfo *client_info = malloc(sizeof(ClientInfo));
        if (!client_info) continue;            // jeśli brak pamięci - ignorujemy lol (jak wszystkie inne problemy w życiu)

        // akceptujemy nowe połączenie klienta
        client_info->sock = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
        if (client_info->sock < 0) {
            perror("accept");
            free(client_info);
            continue;
        }

        // konwertujemy IP klienta na string i zapisujemy w strukturze
        inet_ntop(AF_INET, &client_addr.sin_addr, client_info->ip, sizeof(client_info->ip));
        printf("[+] Nowe połączenie od %s:%d (socket: %d)\n", client_info->ip, ntohs(client_addr.sin_port), client_info->sock);

        // tworzymy wątek obsługujący komunikację z tym klientem
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, client_info);
        pthread_detach(tid);                // odłączamy wątek, żeby zasoby zostały zwolnione po zakończeniu
    }

    close(server_fd);
    return 0;
}
