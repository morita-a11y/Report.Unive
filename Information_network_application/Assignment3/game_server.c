#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <time.h>

#define PORT 20030
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct {
    int fd;
    char name[50];
    int score;
    int attempts;
} Client;

int secret_number;
Client clients[MAX_CLIENTS];

void reset_game() {
    secret_number = rand() % 100 + 1;
    printf("[SERVER] New secret number: %d\n", secret_number);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].attempts = 0;
    }
}

void broadcast(int sender_fd, const char *message) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1 && clients[i].fd != sender_fd) {
            send(clients[i].fd, message, strlen(message), 0);
        }
    }
}

void handle_client_message(int client_index, char *message) {
    Client *client = &clients[client_index];
    int guess = atoi(message);
    client->attempts++;

    char response[BUFFER_SIZE];
    if (guess == secret_number) {
        int points = 11 - client->attempts;
        if (points < 1) points = 1;
        client->score += points;
        snprintf(response, BUFFER_SIZE, "[%s]: %d correct! You got %d points\n", client->name, guess, points);
        send(client->fd, response, strlen(response), 0);
        reset_game();
    } else {
        snprintf(response, BUFFER_SIZE, "[%s]: %s than %d, current points %d\n", client->name,
                 (guess < secret_number) ? "Higher" : "Lower", guess, 10 - client->attempts);
        send(client->fd, response, strlen(response), 0);
    }
}

int main() {
    srand(time(NULL));
    reset_game();

    int server_fd, new_socket, max_fd;
    struct sockaddr_in address;
    fd_set readfds;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, MAX_CLIENTS);
    
    for (int i = 0; i < MAX_CLIENTS; i++) clients[i].fd = -1;
    printf("[SERVER] Listening on port %d...\n", PORT);
    
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_fd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd > 0) {
                FD_SET(clients[i].fd, &readfds);
                if (clients[i].fd > max_fd) max_fd = clients[i].fd;
            }
        }

        select(max_fd + 1, &readfds, NULL, NULL, NULL);
        
        if (FD_ISSET(server_fd, &readfds)) {
            socklen_t addrlen = sizeof(address);
            new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
            printf("[SERVER] New connection accepted\n");
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd == -1) {
                    clients[i].fd = new_socket;
                    strcpy(clients[i].name, "Player");
                    clients[i].score = 0;
                    clients[i].attempts = 0;
                    send(new_socket, "Welcome! Guess a number between 1 and 100.\n", 44, 0);
                    break;
                }
            }
        }
        
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd != -1 && FD_ISSET(clients[i].fd, &readfds)) {
                char buffer[BUFFER_SIZE];
                int valread = read(clients[i].fd, buffer, BUFFER_SIZE);
                if (valread <= 0) {
                    close(clients[i].fd);
                    clients[i].fd = -1;
                    printf("[SERVER] A player disconnected\n");
                } else {
                    buffer[valread] = '\0';
                    handle_client_message(i, buffer);
                }
            }
        }
    }
    return 0;
} 