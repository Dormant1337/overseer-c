#ifndef API_H
#define API_H

#include <pthread.h>
#include <stddef.h>
#include "network.h"

int core_connect(const char *ip, int port);
int core_send_message(const char *ip, int port, const char *payload);
int core_upload_file(const char *ip, int port, const char *path, progress_cb_t cb);
void core_start_scan(pthread_t *thread);

#endif