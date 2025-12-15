#define _XOPEN_SOURCE_EXTENDED
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include "network.h"
#include "../globals.h"

void send_message(const char *ip, int port, const char *msg)
{
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) return;

        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);
        inet_pton(AF_INET, ip, &serv_addr.sin_addr);

        struct timeval timeout = {1, 0};
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == 0) {
                send(sock, msg, strlen(msg), 0);
        }
        close(sock);
}

int connect_handshake(const char *ip, int port)
{
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) return -1;

        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);
        inet_pton(AF_INET, ip, &serv_addr.sin_addr);

        struct timeval timeout = {2, 0};
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                close(sock);
                return -1;
        }

        send(sock, "HANDSHAKE", 9, 0);

        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        char recv_buf[16];
        ssize_t n = recv(sock, recv_buf, 15, 0);
        close(sock);

        return (n >= 0) ? 0 : -1;
}

void *beacon_listener(void *arg)
{
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) pthread_exit(NULL);

        int opt = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(BEACON_PORT);
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
                close(sock);
                pthread_exit(NULL);
        }

        char buffer[BEACON_MSG_SIZE];
        struct sockaddr_in sender_addr;
        socklen_t sender_len;
        struct timeval tv = {0, 100000};
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        while (beacon_thread_active) {
                sender_len = sizeof(sender_addr);
                if (recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&sender_addr, &sender_len) > 0) {
                        char ip_str[16], real_ip[16];
                        int port, id;

                        if (sscanf(buffer, "%15s %d %d", ip_str, &port, &id) == 3) {
                                strcpy(real_ip, inet_ntoa(sender_addr.sin_addr));

                                pthread_mutex_lock(&list_mutex);
                                bool exists = false;
                                for (int i = 0; i < server_count; i++) {
                                        if (strcmp(server_list[i].ip, real_ip) == 0 && server_list[i].port == port) {
                                                exists = true;
                                                server_list[i].server_id = id;
                                                break;
                                        }
                                }
                                if (!exists && server_count < MAX_SERVERS) {
                                        server_list[server_count].server_id = id;
                                        server_list[server_count].port = port;
                                        strcpy(server_list[server_count].ip, real_ip);
                                        sprintf(server_list[server_count].message, "ID: %04d", id);
                                        server_count++;
                                }
                                pthread_mutex_unlock(&list_mutex);
                        }
                }
        }
        close(sock);
        pthread_exit(NULL);
}