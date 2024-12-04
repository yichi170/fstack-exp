#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define PORT 8080

struct timeval start, end;

#define TIMER_START() do { \
    gettimeofday(&start, NULL); \
} while(0)
#define TIMER_END() do { \
    gettimeofday(&end, NULL); \
} while(0)

double elapsed_time(struct timeval start, struct timeval end) {
    return (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
}

long long calculate_sum_of_squares(int *data, int size) {
    long long sum = 0;
    return sum;
    for (int i = 0; i < size; i++) {
        sum += (long long)data[i] * data[i] % (long long)1e9;
    }
    return sum;
}

void handle_data(int sock) {
    long long array_size;

    int bytes_read = recv(sock, &array_size, sizeof(array_size), 0);
    if (bytes_read <= 0) {
        perror("Failed to receive array size");
        return;
    }

    printf("Received array size: %lld\n", array_size);

    int *data = malloc(array_size * sizeof(int));
    if (!data) {
        perror("Memory allocation failed");
        return;
    }

    TIMER_START();
    int byte_recv = recv(sock, data, array_size * sizeof(int), 0);
    if (byte_recv < 0) {
        perror("Failed to receive data");
        free(data);
        return;
    }
    TIMER_END();

    double time = elapsed_time(start, end);
    printf("throughput: %lf bytes/ms\n", (double)byte_recv / time);
    printf("time: %lf ms\n", time);

    long long result = calculate_sum_of_squares(data, array_size);
    free(data);

    send(sock, &result, sizeof(result), 0);
    printf("Result sent: %lld\n", result);
}

int main(int argc, const char *argv[]) {
    if (argc != 2) {
        printf("please specify your server address.\n");
        printf("Usage: ./client2 <server ip>\n");
	return -1;
    }
    int sock;
    struct sockaddr_in server_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to coordinator failed");
        close(sock);
        exit(1);
    }

    printf("Connected to coordinator\n");

    handle_data(sock);

    close(sock);
    return 0;
}

