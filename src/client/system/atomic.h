#ifndef ATOMIC_H
#define ATOMIC_H

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>

typedef struct {
    _Atomic bool connected;
    _Atomic int server_id;
    char ip[16];
    int port;
    pthread_mutex_t validation_lock;
} atomic_server_state_t;

typedef struct {
    void *data;
    size_t size;
    _Atomic bool in_use;
    pthread_mutex_t operation_lock;
} atomic_operation_t;

int atomic_init_server_state(atomic_server_state_t *state, const char *ip, int port, int server_id);
void atomic_update_server_state(atomic_server_state_t *state, bool connected);
bool atomic_validate_and_begin_operation(atomic_operation_t *op);

void atomic_complete_operation(atomic_operation_t *op);
int atomic_copy_and_lock_data(atomic_operation_t *src, atomic_operation_t *dst);

#endif