#ifndef GLOBALS_H
#define GLOBALS_H

#include <pthread.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <ncurses.h>
#include <stdatomic.h>

#define MAX_SERVERS 16
#define BEACON_PORT 9999
#define BEACON_MSG_SIZE 256

struct ServerInfo {
	char message[128];
	char ip[16];
	int port;
	int server_id;
	float cpu_usage;
	size_t mem_used;
	size_t mem_total;
};

extern struct ServerInfo server_list[MAX_SERVERS];
extern struct ServerInfo current_server;
extern volatile int server_count;
extern pthread_mutex_t list_mutex;

extern bool connected_to_server;
extern bool scan_in_progress;
extern int scan_render_cycle;
extern int ui_render_cycle;
extern struct timeval scan_last_time;
extern atomic_bool beacon_thread_active;
extern char connection_password[64];

extern int rows, cols;
extern int target_row_start, target_row_end;
extern int target_cols_start, target_cols_end;
extern bool term_too_small;
extern MEVENT event;
extern int last_click_x, last_click_y;

#endif