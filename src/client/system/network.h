#ifndef NETWORK_H
#define NETWORK_H

void send_message(const char *ip, int port, const char *msg);
int connect_handshake(const char *ip, int port);
void *beacon_listener(void *arg);

#endif