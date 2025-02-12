#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>

#define BUFFER_SIZE 1024

// write() を安全に呼び出す関数
ssize_t safe_write(int fd, const void *buf, size_t count) {
    size_t total_written = 0;
    const char *ptr = buf;

    while (total_written < count) {
        ssize_t bytes_written = write(fd, ptr + total_written, count - total_written);
        if (bytes_written < 0) {
            if (errno == EINTR) {
                continue; // シグナル割り込みの場合は再試行
            }
            perror("write");
            return -1;
        }
        total_written += bytes_written;
    }
    return total_written;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <hostname> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *hostname = argv[1];
    const char *port = argv[2];

    int sockfd;
    struct addrinfo hints, *res, *p;

    // getaddrinfo の設定
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP

    // 名前解決
    int status = getaddrinfo(hostname, port, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    // ソケット作成 & 接続
    for (p = res; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) continue;

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == 0) {
            break; // 接続成功
        }
        close(sockfd);
    }

    if (p == NULL) {
        fprintf(stderr, "Failed to connect to %s:%s\n", hostname, port);
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res); // 名前解決の結果を解放

    // select() を使用して標準入力とサーバーソケットを監視
    fd_set read_fds;
    char buffer[BUFFER_SIZE];

    while (1) {
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

        // サーバーからのデータを標準出力に表示
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

    // ソケットを閉じる
    close(sockfd);
    return 0;
}
