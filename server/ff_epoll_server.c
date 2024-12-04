#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <fcntl.h>

#ifdef FSTACK
    #include "ff_config.h"
    #include "ff_api.h"
    #include "ff_epoll.h"

    #define socket ff_socket
    #define bind ff_bind
    #define listen ff_listen
    #define accept ff_accept
    #define epoll_create ff_epoll_create
    #define epoll_create ff_epoll_create
    #define epoll_ctl ff_epoll_ctl
    #define epoll_wait ff_epoll_wait
    #define sockaddr linux_sockaddr
#endif

#define PORT 8080
#define MAX_EVENTS 10
#define DATA_SIZE (1024LL * 1024LL * 1024LL * 1LL) // 256 million integers = 1GB

void set_fd_non_blocking(int fd) {
#ifndef FSTACK
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL failed");
        exit(EXIT_FAILURE);
    }

    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1) {
        perror("fcntl F_SETFL failed");
        exit(EXIT_FAILURE);
    }
#else
    int on = 1;
    ff_ioctl(fd, FIONBIO, &on);
#endif
}

int create_server_sock() {
#ifdef FSTACK
    char *ffargv[4] = {
        "./ff_epoll_server",
        "--conf=/data/f-stack/config.ini",
        "--proc-type=primary",
        "--proc-id=0"
    };
    ff_init(4, ffargv);
#endif
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    set_fd_non_blocking(sockfd);

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 10) == -1) {
        perror("listen failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

int main() {
    int server_fd = create_server_sock();
    printf("Server listening on port %d...\n", PORT);

    int epoll_fd = epoll_create(1);
    if (epoll_fd == -1) {
        perror("epoll_create1 failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        perror("epoll_ctl failed");
        close(server_fd);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    int *data = (int *)malloc(DATA_SIZE * sizeof(int));
    if (!data) {
        perror("malloc failed");
        close(server_fd);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    for (long long i = 0; i < DATA_SIZE; ++i) {
        data[i] = i;
    }

    struct epoll_event events[MAX_EVENTS];
    while (1) {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_events == -1) {
            perror("epoll_wait failed");
            break;
        }

        for (int i = 0; i < num_events; ++i) {
            if (events[i].data.fd == server_fd) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
                if (client_fd == -1) {
                    perror("accept failed");
                    continue;
                } else {
		    printf("accept client!\n");
		}

                ev.events = EPOLLOUT | EPOLLET; // Register for write events
                ev.data.fd = client_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                    perror("epoll_ctl failed");
                    close(client_fd);
                    continue;
                }
                printf("New client connected\n");
            } else if (events[i].events & EPOLLOUT) {
                // Send entire 1GB of data to the client
                int client_fd = events[i].data.fd;
		long long data_size = DATA_SIZE;
                ssize_t bytes_sent = send(client_fd, &data_size, sizeof(data_size), 0);
                bytes_sent = send(client_fd, data, DATA_SIZE * sizeof(int), 0);
                if (bytes_sent == -1) {
                    perror("send failed");
                    continue;
                }

                // All data has been sent, close connection
                printf("Sent 1GB data to client\n");
                close(client_fd);
            }
        }
    }

    free(data);
    close(server_fd);
    close(epoll_fd);
    return 0;
}

