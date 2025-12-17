#ifndef NETWORK_H
#define NETWORK_H

#include <stddef.h>

typedef void (*progress_cb_t)(size_t sent, size_t total, double speed_mbps);

void send_message(const char *ip, int port, const char *msg);
int send_file_to_server(const char *ip, int port, const char *filepath, progress_cb_t callback);
int connect_handshake(const char *ip, int port);
void *beacon_listener(void *arg);

#endif