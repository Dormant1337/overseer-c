#define _XOPEN_SOURCE_EXTENDED
#include "../globals.h"
#include "../system/api.h"
#include "interface.h"
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

int safe_getnstr(char *buf, size_t buf_size, int max_chars)
{
	if (buf == NULL)
		return -2;

	int limit =
	    (max_chars < (int)buf_size - 1) ? max_chars : (int)buf_size - 1;

	if (limit <= 0)
		return -1;
	getnstr(buf, limit);

	buf[buf_size - 1] = '\0';
	return 0;
}

void safe_popup_dimensions(int *w, int *h, int *x, int *y)
{
	*w = (*w < 30) ? 30 : *w;
	*h = (*h < 6) ? 6 : *h;

	if (*w > cols - 4)
		*w = cols - 4;
	if (*h > rows - 4)
		*h = rows - 4;

	*y = (rows - *h) / 2;
	*x = (cols - *w) / 2;

	if (*y < 0)
		*y = 0;
	if (*x < 0)
		*x = 0;

	if (*y + *h > rows)
		*y = rows - *h;
	if (*x + *w > cols)
		*x = cols - *w;
}

void handle_input_btop(pthread_t *thread_ptr)
{
	int box_w = target_cols_end - target_cols_start;

	if (!connected_to_server && !scan_in_progress) {
		int btn_w = 16;
		int btn_x = target_cols_start + box_w - btn_w - 2;
		int btn_y = target_row_start;

		if (last_click_y >= btn_y && last_click_y <= btn_y + 2 &&
		    last_click_x >= btn_x && last_click_x <= btn_x + btn_w) {

			scan_in_progress = true;
			scan_render_cycle = 0;
			gettimeofday(&scan_last_time, NULL);

			core_start_scan(thread_ptr);
			return;
		}

		int list_start_y = target_row_start + 4;
		int clicked_index = -1;

		pthread_mutex_lock(&list_mutex);
		for (int i = 0; i < server_count; i++) {
			if (last_click_y == list_start_y + i &&
			    last_click_x > target_cols_start
			    && last_click_x < target_cols_end) {
				current_server = server_list[i];
				clicked_index = i;
				break;
			}
		}
		pthread_mutex_unlock(&list_mutex);

		if (clicked_index != -1) {
			int w = 34, h = 8;
			int cy = rows / 2 - h / 2, cx = cols / 2 - w / 2;

			attron(COLOR_PAIR(CP_DEFAULT));
			for (int k = 0; k < h; k++) {
				mvhline(cy + k, cx, ' ', w);
			}
			draw_btop_box(cy, cx, h, w, "AUTHENTICATION");

			mvprintw(cy + 2, cx + 2, "ENTER PASSWORD:");

			attron(A_REVERSE);
			mvhline(cy + 4, cx + 2, ' ', w - 4);
			attroff(A_REVERSE);

			echo();
			curs_set(1);

			char pass_buf[64] = { 0 };
			move(cy + 4, cx + 2);
			timeout(-1);

			safe_getnstr(pass_buf, sizeof(pass_buf), w - 4 - 1);
			timeout(10);
			noecho();
			curs_set(0);

			draw_spinner(cy + 2, cx + w / 2 - 4);
			attron(A_BLINK);
			mvprintw(cy + 6, cx + 2, " CONNECTING...        ");
			attroff(A_BLINK);
			attroff(COLOR_PAIR(CP_DEFAULT));
			refresh();

			usleep(500000);

			if (core_connect
			    (current_server.ip, current_server.port,
			     pass_buf) == 0) {
				connected_to_server = true;
			} else {
				attron(COLOR_PAIR(CP_WARN) | A_BOLD);
				mvprintw(cy + 6, cx + 2,
					 " ACCESS DENIED        ");
				attroff(COLOR_PAIR(CP_WARN) | A_BOLD);
				refresh();
				usleep(500000);
			}
		}
		return;
	}

	if (connected_to_server) {
		int btn_w = 20;
		int btn_start_x = target_cols_start + 4;
		int btn_msg_y = target_row_start + 8;
		int btn_file_y = target_row_start + 12;
		int btn_exec_y = target_row_start + 16;
		int btn_disc_y = target_row_start + 20;

		if (last_click_y >= btn_msg_y && last_click_y < btn_msg_y + 3 &&
		    last_click_x >= btn_start_x
		    && last_click_x < btn_start_x + btn_w) {
			popup_input_btop();
		}

		if (last_click_y >= btn_file_y && last_click_y < btn_file_y + 3
		    && last_click_x >= btn_start_x
		    && last_click_x < btn_start_x + btn_w) {
			popup_file_upload();
		}

		if (last_click_y >= btn_exec_y && last_click_y < btn_exec_y + 3
		    && last_click_x >= btn_start_x
		    && last_click_x < btn_start_x + btn_w) {
			popup_execute_cmd();
		}

		if (last_click_y >= btn_disc_y && last_click_y < btn_disc_y + 3
		    && last_click_x >= btn_start_x
		    && last_click_x < btn_start_x + btn_w) {
			connected_to_server = false;
			memset(connection_password, 0,
			       sizeof(connection_password));
		}
	}
}
