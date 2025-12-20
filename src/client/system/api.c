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