#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include "ff_api.h"
#include "ff_epoll.h"

#define SERVER_IP "10.10.1.2"
#define SERVER_PORT 8080
#define MAX_EVENTS 10

long long calculate_sum(int *data, long long array_size) {
    return 0LL;
}

int event_loop(void *arg) {
    int epollfd = *(int *)arg;
    struct epoll_event events[MAX_EVENTS];
    char recv_buffer[2048];
    long long array_size = 0;
    static int *data = NULL;
    ssize_t bytes_received;
    long long sum = 0;
    
    int nfds = ff_epoll_wait(epollfd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
        perror("ff_epoll_wait failed");
        return -1;
    }

    for (int n = 0; n < nfds; ++n) {
        int sockfd = events[n].data.fd;
        if (events[n].events & EPOLLIN) {
            printf("Socket ready to receive data...\n");

            bytes_received = ff_recv(sockfd, &array_size, sizeof(array_size), 0);
            if (bytes_received < 0) {
                perror("Receive failed");
                return -1;
            }

            printf("Array size received: %lld\n", array_size);

            data = malloc(array_size * sizeof(int));

            if (!data) {
                perror("Failed to allocate memory for received data");
                return -1;
            }

            bytes_received = ff_recv(sockfd, data, array_size * sizeof(int), 0);

            if (bytes_received < 0) {
                perror("Receive failed");
                free(data);
                return -1;
            } else if (bytes_received == 0) {
                printf("Server closed the connection.\n");
                free(data);
                ff_close(sockfd);
                return 0;
            } else {
                printf("Received %lld integers from server\n", array_size);
                sum = calculate_sum(data, array_size);
                printf("Calculated sum of received data: %lld\n", sum);
            }
        }
        if (events[n].events & EPOLLOUT) {
            if (array_size > 0 && data != NULL) {
                ssize_t bytes_sent = ff_send(sockfd, &sum, sizeof(sum), 0);
                if (bytes_sent < 0) {
                    perror("Failed to send result back to server");
                } else {
                    printf("Sent result back to server: %lld\n", sum);
                    free(data);
                    data = NULL;
                    array_size = 0;
                    sum = 0;
                    ff_close(sockfd);
                    printf("Connection closed.\n");
                }
            }
        }
    }
    return 0;
}

int main() {
    int sockfd, epollfd;
    struct sockaddr_in server_addr;
    struct epoll_event ev;
    
    // Step 1: Initialize F-Stack
    char *ffargv[4] = {
                "./ffclient",
                "--conf=/data/f-stack/config.ini",
                "--proc-type=primary",
                "--proc-id=0"
        };
    ff_init(4, ffargv);

    // Step 2: Create socket using F-Stack API
    sockfd = ff_socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ff_socket creation failed");
        exit(EXIT_FAILURE);
    }

    printf("Socket created successfully.\n");

    // Step 3: Set the socket to non-blocking mode
    int on = 1;
    if (ff_ioctl(sockfd, FIONBIO, &on) < 0) {
        ff_close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Step 4: Set up the server address struct
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address / Address not supported");
        ff_close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Step 5: Connect to the server (non-blocking)
    if (ff_connect(sockfd, (struct linux_sockaddr *)&server_addr, sizeof(server_addr)) < 0 && errno != EINPROGRESS) {
        if (errno != EINPROGRESS) {
            perror("Connection failed");
            ff_close(sockfd);
            exit(EXIT_FAILURE);
        }
    }
    printf("Connection initiated...\n");

    // Step 6: Create an epoll instance
    epollfd = ff_epoll_create(1);
    if (epollfd == -1) {
        perror("ff_epoll_create failed");
        ff_close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Step 7: Register the socket with epoll for both read and write events
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    ev.data.fd = sockfd;
    if (ff_epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev) == -1) {
        perror("ff_epoll_ctl failed");
        ff_close(sockfd);
        ff_close(epollfd);
        exit(EXIT_FAILURE);
    }

    // Step 8: Run the event loop
    ff_run(event_loop, &epollfd);

    ff_close(sockfd);
    ff_close(epollfd);
    return 0;
}

