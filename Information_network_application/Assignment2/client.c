#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

#define BUFFER_SIZE BUFSIZ

ssize_t safe_write(int fd, const void *buf, size_t count) {
    ssize_t written = 0, total_written = 0;
    while (total_written < count) {
        written = write(fd, (char *)buf + total_written, count - total_written);
        if (written < 0) {
            if (errno == EINTR) continue; // シグナル割り込みなら再試行
            perror("write");
            return -1;
        }
        total_written += written;
    }
    return total_written;
}

int main (int argc, char *argv[]) {
    int sockfd;
    char buffer[BUFFER_SIZE];
    char mesg[BUFFER_SIZE] = "Hello, world!\n";
    char *portname = "10000";
    char *servername = "localhost";
    int error;
    struct addrinfo hints, *res, *r;

    if (argc >= 2) {
        servername = argv[1];    /* 第1引数: サーバー名 */
        if (argc >= 3) {
            portname = argv[2];  /* 第2引数: ポート名 */
            if (argc >= 4) {
                snprintf(mesg, BUFFER_SIZE, "%s\n", argv[3]); /* 第3引数: 送信文字列 */
            }
        }
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;       /* IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;   /* TCP */
    if ((error = getaddrinfo(servername, portname, &hints, &res))) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        exit(1);
    }

    for (r = res; r; r = r->ai_next) {
        if ((sockfd = socket(r->ai_family, r->ai_socktype, r->ai_protocol)) < 0)
            continue;
        if (connect(sockfd, r->ai_addr, r->ai_addrlen) == 0)
            break;
        close(sockfd);
        sockfd = -1;
    }
    freeaddrinfo(res);
    if (sockfd < 0) {
        perror("Failed to connect");
        exit(1);
    }

    fd_set read_fds;
    
    for (;;) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(sockfd, &read_fds);

        int max_fd = (sockfd > STDIN_FILENO) ? sockfd : STDIN_FILENO;
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) {
            if (errno == EINTR) continue; // シグナル割り込みなら再試行
            perror("select");
            break;
        }

        // 標準入力からの入力をサーバーへ送信
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            ssize_t bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE);
            if (bytes_read < 0) {
                perror("read (stdin)");
                break;
            } else if (bytes_read == 0) {
                printf("Standard input closed.\n");
                break;
            }
            if (safe_write(sockfd, buffer, bytes_read) < 0) break;
        }

        if (FD_ISSET(sockfd, &read_fds)) {
            ssize_t bytes_received = read(sockfd, buffer, BUFFER_SIZE);
            if (bytes_received < 0) {
                perror("read (socket)");
                break;
            } else if (bytes_received == 0) {
                printf("Server closed the connection.\n");
                break;
            }
            if (safe_write(STDOUT_FILENO, buffer, bytes_received) < 0) break;
        }
    }

    close(sockfd);
    return 0;
}
