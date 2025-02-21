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

Client clients[MAX_CLIENTS];
int secret_number;

void reset_game() {
    secret_number = rand() % 100 + 1;
    printf("New number: %d\n", secret_number);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].attempts = 0;
    }
}

void broadcast(int sender_fd, const char *message) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd > 0 && clients[i].fd != sender_fd) {
            send(clients[i].fd, message, strlen(message), 0);
        }
    }
}

void remove_client(int index) {
    printf("Client %s left.\n", clients[index].name);
    close(clients[index].fd);
    clients[index].fd = 0;
    clients[index].name[0] = '\0';
    clients[index].score = 0;
    clients[index].attempts = 0;
}

int main() {
    srand(time(NULL));
    reset_game();
    
    int server_fd, max_fd, activity, new_socket, addrlen, valread;
    struct sockaddr_in address;
    fd_set readfds;
    char buffer[BUFFER_SIZE];
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server started on port %d\n", PORT);
    for(;;) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_fd = server_fd;
        
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd > 0) {
                FD_SET(clients[i].fd, &readfds);
                if (clients[i].fd > max_fd) max_fd = clients[i].fd;
            }
        }
        
        activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0)) {
            perror("Select error");
        }
        
        if (FD_ISSET(server_fd, &readfds)) {
            addrlen = sizeof(address);
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd == 0) {
                    clients[i].fd = new_socket;
                    clients[i].score = 0;
                    clients[i].attempts = 0;
                    send(new_socket, "Enter your name: ", 17, 0);
                    valread = read(new_socket, clients[i].name, 50);
                    clients[i].name[valread - 1] = '\0'; // Remove newline character
                    printf("New connection: %s\n", clients[i].name);
                    break;
                }
            }
        }
        
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int client_fd = clients[i].fd;
            if (client_fd > 0 && FD_ISSET(client_fd, &readfds)) {
                memset(buffer, 0, BUFFER_SIZE);
                if ((valread = read(client_fd, buffer, BUFFER_SIZE)) == 0) {
                    remove_client(i);
                } else {
                    buffer[valread - 1] = '\0';
                    
                    if (strcmp(buffer, "exit") == 0) {
                        remove_client(i);
                        continue;
                    }

                    int guess = atoi(buffer);
                    clients[i].attempts++;
                    char response[BUFFER_SIZE];
                    if (guess == secret_number) {
                        int points = 11 - clients[i].attempts;
                        clients[i].score += points;
                        snprintf(response, BUFFER_SIZE, "[%s]: %d correct! You got %d points\n", clients[i].name, guess, points);
                        broadcast(client_fd, response);
                        reset_game();
                    } else {
                        snprintf(response, BUFFER_SIZE, "[%s]: %s than %d, current point %d\n", clients[i].name, guess < secret_number ? "Higher" : "Lower", guess, 10 - clients[i].attempts);
                        send(client_fd, response, strlen(response), 0);
                    }
                }
            }
        }
    }
    return 0;
}
