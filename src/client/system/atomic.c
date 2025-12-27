#define _XOPEN_SOURCE_EXTENDED
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include "atomic.h"

int atomic_init_server_state(atomic_server_state_t* state, const char* ip, int port, int server_id)
{
	if (!state || !ip) return -1;
	memset(state, 0, sizeof(atomic_server_state_t));

	atomic_store(&state->connected, false);
	atomic_store(&state->server_id, server_id);

	strncpy(state->ip, ip, sizeof(state->ip) - 1);
	state->ip[sizeof(state->ip) - 1] = '\0';
	state->port = port;

	if (pthread_mutex_init(&state->validation_lock, NULL) != 0)
		return -1;

	return 0;
}

void atomic_update_server_state(atomic_server_state_t* state, bool connected)
{
	if (!state) return;
	atomic_store(&state->connected, connected);
}

bool atomic_validate_and_begin_operation(atomic_operation_t* op)
{
	if (!op) return false;
	bool expected = false;
	return atomic_compare_exchange_strong(&op->in_use, &expected, true);
}

void atomic_complete_operation(atomic_operation_t* op)
{
	if (!op) return;
	atomic_store(&op->in_use, false);
}

int atomic_copy_and_lock_data(atomic_operation_t* src, atomic_operation_t* dst)
{
	if (!src || !dst) return -1;
	pthread_mutex_lock(&src->operation_lock);

	if (!src->data || src->size == 0) {
		pthread_mutex_unlock(&src->operation_lock);
		return -1;
	}

	pthread_mutex_lock(&dst->operation_lock);

	if (!dst->data || dst->size < src->size) {
		void *new_data = realloc(dst->data, src->size);

		if (!new_data) {
			pthread_mutex_unlock(&dst->operation_lock);
			pthread_mutex_unlock(&src->operation_lock);
			return -1;
		}

		dst->data = new_data;
		dst->size = src->size;
	}

	memcpy(dst->data, src->data, src->size);
	pthread_mutex_unlock(&dst->operation_lock);
	pthread_mutex_unlock(&src->operation_lock);
	return 0;
}