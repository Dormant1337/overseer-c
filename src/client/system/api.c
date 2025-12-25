#define _XOPEN_SOURCE_EXTENDED
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdatomic.h>
#include "api.h"
#include "network.h"
#include "../globals.h"

int core_connect(const char *ip, int port)
{
	if (!ip || port <= 0 || port > 65535) return -1;
	return connect_handshake(ip, port);
}

int core_send_message(const char *ip, int port, const char *payload)
{
	if (!ip || !payload) return -1;
	size_t len = strlen(payload);
	if (len == 0 || len > 1024) return -1;

	send_message(ip, port, payload);
	return 0;
}

int core_upload_file(const char *ip, int port, const char *path, progress_cb_t cb)
{
	if (!ip || !path) return -1;
	
	struct stat st;
	if (stat(path, &st) != 0) return -1;
	if (!S_ISREG(st.st_mode)) return -1;
	if (st.st_size == 0) return -1;

	return send_file_to_server(ip, port, path, cb);
}

void core_start_scan(pthread_t *thread)
{
	pthread_mutex_lock(&list_mutex);
	server_count = 0;
	pthread_mutex_unlock(&list_mutex);

	atomic_store(&beacon_thread_active, true);

	if (*thread) pthread_join(*thread, NULL);
	pthread_create(thread, NULL, beacon_listener, NULL);
}

int core_send_message_atomic(const char* ip, int port, safe_buffer_t* buf)
{
	if (!ip || !buf) return -1;
	char* payload_copy = NULL;
	size_t payload_len = 0;

	char ip_copy[16] = {0};
	int port_copy = port;
	pthread_mutex_lock(&buf->lock);

	if (!buf->data || buf->length == 0 || buf->length > 1024)
	{
		pthread_mutex_unlock(&buf->lock);
		return -1;
	}

	payload_copy = malloc(buf->length + 1);

	if (!payload_copy)
	{
		pthread_mutex_unlock(&buf->lock);
		return -1;
	}

	memcpy(payload_copy, buf->data, buf->length);
	payload_copy[buf->length] = '\0';

	payload_len = buf->length;
	pthread_mutex_unlock(&buf->lock);

	if (port_copy <= 0 || port_copy > 65535)
	{
		free(payload_copy);
		return -1;
	}

	strncpy(ip_copy, ip, sizeof(ip_copy) - 1);
	ip_copy[sizeof(ip_copy) - 1] = '\0';

	send_message(ip_copy, port_copy, payload_copy);
	free(payload_copy);
	return 0;
}

int core_upload_file_atomic(const char* ip, int port, safe_buffer_t* path_buf, progress_cb_t cb)
{
	if (!ip || !path_buf) return -1;
	char* path_copy = NULL;

	char ip_copy[16] = {0};
	int port_copy = port;
	pthread_mutex_lock(&path_buf->lock);

	if (!path_buf->data || path_buf->length == 0 || path_buf->length > PATH_MAX)
	{
		pthread_mutex_unlock(&path_buf->lock);
		return -1;
	}

	path_copy = strndup(path_buf->data, path_buf->length);

	if (!path_copy)
	{
		pthread_mutex_unlock(&path_buf->lock);
		return -1;
	}

	pthread_mutex_unlock(&path_buf->lock);

	if (port_copy <= 0 || port_copy > 65535)
	{
		free(path_copy);
		return -1;
	}

	strncpy(ip_copy, ip, sizeof(ip_copy) - 1);
	ip_copy[sizeof(ip_copy) - 1] = '\0';
	struct stat st;

	if (stat(path_copy, &st) != 0)
	{
		free(path_copy);
		return -1;
	}

	if (!S_ISREG(st.st_mode))
	{
		free(path_copy);
		return -1;
	}

	if (st.st_size == 0)
	{
		free(path_copy);
		return -1;
	}

	int result = send_file_to_server(ip_copy, port_copy, path_copy, cb);
	free(path_copy);
	return result;
}

int core_init_safe_buffer(safe_buffer_t* buf, size_t initial_capacity)
{
	if (!buf) return -1;
	memset(buf, 0, sizeof(safe_buffer_t));

	if (pthread_mutex_init(&buf->lock, NULL) != 0)
		return -1;

	if (initial_capacity > 0)
	{
		buf->data = malloc(initial_capacity);

		if (!buf->data)
		{
			pthread_mutex_destroy(&buf->lock);
			return -1;
		}

		buf->capacity = initial_capacity;
	}

	return 0;
}

int core_set_safe_buffer(safe_buffer_t* buf, const char* data, size_t length)
{
	if (!buf || !data) return -1;
	pthread_mutex_lock(&buf->lock);

	if (buf->capacity < length + 1)
	{
		char *new_data = (char*)realloc(buf->data, length + 1);

		if (!new_data)
		{
			pthread_mutex_unlock(&buf->lock);
			return -1;
		}

		buf->data = new_data;
		buf->capacity = length + 1;
	}

	memcpy(buf->data, data, length);
	buf->data[length] = '\0';

	buf->length = length;
	pthread_mutex_unlock(&buf->lock);
	return 0;
}

void core_clear_safe_buffer(safe_buffer_t* buf)
{
	if (!buf) return;
	pthread_mutex_lock(&buf->lock);

	if (buf->data)
		memset(buf->data, 0, buf->capacity);

	buf->length = 0;
	pthread_mutex_unlock(&buf->lock);
}

void core_destroy_safe_buffer(safe_buffer_t* buf)
{
	if (!buf) return;
	pthread_mutex_lock(&buf->lock);

	if (buf->data)
	{
		free(buf->data);
		buf->data = NULL;
	}

	buf->capacity = 0;
	buf->length = 0;

	pthread_mutex_unlock(&buf->lock);
	pthread_mutex_destroy(&buf->lock);
}

int core_validate_server_state(const char* ip, int port, bool* is_valid)
{
	if (!ip || !is_valid)
		return -1;

	pthread_mutex_lock(&list_mutex);
	bool found = false;

	for (int i = 0; i < server_count; i++)
	{
		if (strcmp(server_list[i].ip, ip) == 0 && server_list[i].port == port)
		{
			found = true;
			break;
		}
	}

	bool is_connected = connected_to_server;
	pthread_mutex_unlock(&list_mutex);

	*is_valid = (found && is_connected);
	return 0;
}