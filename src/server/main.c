#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include <stdarg.h>

#define BEACON_PORT 9999
#define BEACON_MSG_SIZE 256

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

int server_id = 0;
int tcp_port = 8080;
char beacon_msg[BEACON_MSG_SIZE];
volatile bool running = true;
int server_socket_fd = -1;

void log_msg(const char *color, const char *format, ...)
{
        time_t rawtime;
        struct tm *timeinfo;
        char buffer[80];

        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);

        printf("%s[%s] ", KNRM, buffer);
        printf("%s", color);

        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);

        printf("%s\n", KNRM);
}

void handle_signal(int sig)
{
        printf("\n");
        log_msg(KRED, "Received shutdown signal...");
        running = false;
        if (server_socket_fd != -1) {
                close(server_socket_fd);
        }
}

int form_message(void)
{
        server_id = rand() % 9000 + 1000;
        snprintf(beacon_msg, sizeof(beacon_msg), "0.0.0.0 %d %d", tcp_port, server_id);
        return 0;
}

int setup_server(int port)
{
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) return -1;

        int opt = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
                log_msg(KRED, "Error: Could not bind to port %d", port);
                return -1;
        }
        if (listen(sockfd, 5) < 0) {
                log_msg(KRED, "Error: Could not listen");
                return -1;
        }
        return sockfd;
}

void *send_beacon_thread(void *arg)
{
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) pthread_exit(NULL);

        int broadcastEnable = 1;
        setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(BEACON_PORT);
        addr.sin_addr.s_addr = inet_addr("255.255.255.255");

        log_msg(KMAG, "Beacon Broadcasting on UDP port %d", BEACON_PORT);

        while (running) {
                sendto(sock, beacon_msg, strlen(beacon_msg), 0, (struct sockaddr *)&addr, sizeof(addr));
                sleep(2);
        }
        close(sock);
        pthread_exit(NULL);
}

void handle_client(int client_fd, struct sockaddr_in client_addr)
{
        char buf[256];
        char *client_ip = inet_ntoa(client_addr.sin_addr);

        ssize_t n = recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (n > 0) {
                buf[n] = '\0';
                log_msg(KCYN, "CMD from %s: %s", client_ip, buf);

                const char *response = "ACK: Command Received";
                send(client_fd, response, strlen(response), 0);
        }
        close(client_fd);
}

int main(int argc, char *argv[])
{
        srand((unsigned int)time(NULL));
        signal(SIGINT, handle_signal);

        if (argc > 1) {
                tcp_port = atoi(argv[1]);
        }

        log_msg(KWHT, "--- SYSTEM BOOT ---");
        form_message();

        pthread_t beacon_thread;
        if (pthread_create(&beacon_thread, NULL, send_beacon_thread, NULL) != 0) {
                return 1;
        }

        server_socket_fd = setup_server(tcp_port);
        if (server_socket_fd < 0) {
                running = false;
                pthread_join(beacon_thread, NULL);
                return 1;
        }

        log_msg(KGRN, "TCP Server Listening on port %d (ID: %d)", tcp_port, server_id);

        while (running) {
                struct sockaddr_in client_addr;
                socklen_t len = sizeof(client_addr);

                int client_fd = accept(server_socket_fd, (struct sockaddr *)&client_addr, &len);
                if (client_fd >= 0) {
                        handle_client(client_fd, client_addr);
                }
        }

        log_msg(KYEL, "System Shutdown Complete.");
        close(server_socket_fd);
        pthread_join(beacon_thread, NULL);
        return 0;
}