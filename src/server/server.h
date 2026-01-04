#ifndef OVERSEER_SERVER_H
#define OVERSEER_SERVER_H

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
#include <sys/stat.h>
#include <errno.h>

#define BEACON_PORT		9999
#define BEACON_MSG_SIZE		256

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

extern int server_id;
extern int tcp_port;
extern char *server_password;
extern char beacon_msg[BEACON_MSG_SIZE];
extern volatile bool running;
extern int server_socket_fd;

void log_msg(const char *color, const char *format, ...);
int setup_server(int port);
void *send_beacon_thread(void *arg);
int form_message(void);
void get_sys_stats(char *buffer, size_t size);
void handle_client(int client_fd, struct sockaddr_in client_addr);

#endif
