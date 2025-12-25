#ifndef API_H
#define API_H

#include <pthread.h>
#include <stddef.h>
#include "network.h"

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
    pthread_mutex_t lock;
} safe_buffer_t;

int core_connect(const char *ip, int port);
int core_send_message(const char *ip, int port, const char *payload);
int core_upload_file(const char *ip, int port, const char *path, progress_cb_t cb);
void core_start_scan(pthread_t *thread);

int core_send_message_atomic(const char* ip, int port, safe_buffer_t* buf);
int core_upload_file_atomic(const char* ip, int port, safe_buffer_t* path_buf, progress_cb_t cb);

int core_init_safe_buffer(safe_buffer_t* buf, size_t initial_capacity);
int core_set_safe_buffer(safe_buffer_t* buf, const char *data, size_t length);

void core_clear_safe_buffer(safe_buffer_t* buf);
void core_destroy_safe_buffer(safe_buffer_t* buf);
int core_validate_server_state(const char* ip, int port, bool* is_valid);
#endif