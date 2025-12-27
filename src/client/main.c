#define _XOPEN_SOURCE_EXTENDED
#include <ncurses.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdatomic.h>
#include "globals.h"
#include "tui/interface.h"
#include "system/api.h" 

pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
struct ServerInfo server_list[MAX_SERVERS];
struct ServerInfo current_server;
volatile int server_count = 0;

bool connected_to_server = false;
bool scan_in_progress = false;
int scan_render_cycle = 0;
int ui_render_cycle = 0;
struct timeval scan_last_time;
struct timeval ui_last_time;
atomic_bool beacon_thread_active = false;

int rows, cols;
int target_row_start, target_row_end;
int target_cols_start, target_cols_end;
bool term_too_small = false;
MEVENT event;
int last_click_x, last_click_y;

int main(void)
{
	setlocale(LC_ALL, "");
	initscr();
	cbreak();
	noecho();
	curs_set(0);
	keypad(stdscr, TRUE);
	mouseinterval(0);
	init_colors();
	timeout(10);
	mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
	printf("\033[?1003h\n");

	pthread_t beacon_thread = 0;
	gettimeofday(&scan_last_time, NULL);
	gettimeofday(&ui_last_time, NULL);

	atomic_store(&beacon_thread_active, false);

	while (1) {
		getmaxyx(stdscr, rows, cols);
		int ch = getch();

		if (ch == 'q') break;
		if (ch == KEY_MOUSE && getmouse(&event) == OK) {
			if (event.bstate & (BUTTON1_PRESSED | BUTTON1_CLICKED)) {
				last_click_x = event.x;
				last_click_y = event.y;
				handle_input_btop(&beacon_thread);
			}
		}

		target_row_start = rows * 0.15;
		target_row_end = rows * 0.85;
		target_cols_start = cols * 0.15;
		target_cols_end = cols * 0.85;
		int box_h = target_row_end - target_row_start;
		int box_w = target_cols_end - target_cols_start;

		term_too_small = (rows < 24 || cols < 80);

		erase();
		draw_background();

		if (term_too_small) {
			draw_btop_box(rows/2-4, cols/2-10, 8, 20, "ERROR");
			attron(COLOR_PAIR(CP_WARN) | A_BOLD);
			mvprintw(rows/2+3, cols/2-7, "SIZE ERROR");
			attroff(COLOR_PAIR(CP_WARN) | A_BOLD);
			
			draw_spinner(rows/2-2, cols/2-4);
		} else {
			draw_btop_box(target_row_start, target_cols_start, box_h, box_w, 
				      connected_to_server ? " ACTIVE SESSION " : " NETWORK OVERVIEW ");

			if (!connected_to_server) {
				if (!scan_in_progress) {
					draw_server_table();
					draw_button_btop(target_row_start, target_cols_start + box_w - 18, 16, "REFRESH", false);
				} else {
					attron(COLOR_PAIR(CP_DEFAULT) | A_BOLD);
					mvprintw(rows/2 - 4, cols/2 - 4, "SCANNING");
					attroff(COLOR_PAIR(CP_DEFAULT) | A_BOLD);
					
					draw_spinner(rows/2 - 2, cols/2 - 4);
					
					int pct = (scan_render_cycle * 100) / 20;
					draw_meter(rows/2 + 3, cols/2 - 20, 40, pct);
				}
			} else {
				attron(COLOR_PAIR(CP_DEFAULT));
				mvprintw(target_row_start + 2, target_cols_start + 3, "TARGET NODE:");
				attron(A_BOLD);
				mvprintw(target_row_start + 3, target_cols_start + 3, "%s:%d", current_server.ip, current_server.port);
				attroff(A_BOLD);
				mvprintw(target_row_start + 5, target_cols_start + 3, "NODE ID: %d", current_server.server_id);

				int chart_x = target_cols_end - 35;
				draw_btop_box(target_row_start + 2, chart_x, 10, 30, "TELEMETRY");
				
				attron(COLOR_PAIR(CP_DIM));
				for(int i=0; i<6; i++) {
					mvprintw(target_row_start+3+i, chart_x+1, " . . . . . . . . . . . . . ");
				}
				attroff(COLOR_PAIR(CP_DIM));

				draw_button_btop(target_row_start + 8, target_cols_start + 4, 20, "SEND PAYLOAD", true);
				draw_button_btop(target_row_start + 12, target_cols_start + 4, 20, "SEND FILE", true);
				draw_button_btop(target_row_start + 16, target_cols_start + 4, 20, "EXECUTE CMD", true);
				draw_button_btop(target_row_start + 20, target_cols_start + 4, 20, "TERMINATE", false);
			}
		}

		struct timeval now;
		gettimeofday(&now, NULL);

		long ui_ms = (now.tv_sec - ui_last_time.tv_sec) * 1000 + (now.tv_usec - ui_last_time.tv_usec) / 1000;
		if (ui_ms > 80) {
			ui_render_cycle++;
			ui_last_time = now;
		}

		if (scan_in_progress) {
			long ms = (now.tv_sec - scan_last_time.tv_sec) * 1000 + (now.tv_usec - scan_last_time.tv_usec) / 1000;
			if (ms > 100) {
				scan_render_cycle++;
				scan_last_time = now;
				if (scan_render_cycle > 20) {
					scan_in_progress = false;
					atomic_store(&beacon_thread_active, false);
					if (beacon_thread) pthread_join(beacon_thread, NULL);
				}
			}
		}

		refresh();
	}

	atomic_store(&beacon_thread_active, false);
	if (beacon_thread) pthread_join(beacon_thread, NULL);
	endwin();
	printf("\033[?1003l\n");
	return 0;
}