#define _XOPEN_SOURCE_EXTENDED
#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdatomic.h>
#include "interface.h"
#include "path_security.h"
#include "../globals.h"
#include "../system/api.h"

#define INPUT_BUFFER_SIZE 256
#define FILEPATH_BUFFER_SIZE 512

#ifndef UPLOAD_BASE_DIR
#define UPLOAD_BASE_DIR "./uploads"
#endif

static int safe_getnstr(char *buf, size_t buf_size, int max_chars);
static void safe_popup_dimensions(int *w, int *h, int *x, int *y);

static const char *spinner_frames[25][4] = {
	{ "┌──     ", "        ", "        ", "        " },
	{ " ───    ", "        ", "        ", "        " },
	{ "  ───   ", "        ", "        ", "        " },
	{ "   ───  ", "        ", "        ", "        " },
	{ "    ─── ", "        ", "        ", "        " },
	{ "     ──┐", "        ", "        ", "        " },
	{ "      ─┐", "       ╵", "        ", "        " },
	{ "       ┐", "       │", "        ", "        " },
	{ "        ", "       │", "       ╵", "        " },
	{ "        ", "       ╷", "       │", "        " },
	{ "        ", "        ", "       │", "       ╵" },
	{ "        ", "        ", "       ╷", "       ┘" },
	{ "        ", "        ", "        ", "      ─┘" },
	{ "        ", "        ", "        ", "     ──┘" },
	{ "        ", "        ", "        ", "    ─── " },
	{ "        ", "        ", "        ", "   ───  " },
	{ "        ", "        ", "        ", "  ───   " },
	{ "        ", "        ", "        ", " ───    " },
	{ "        ", "        ", "        ", "└──     " },
	{ "        ", "        ", "        ", "╷       " },
	{ "        ", "        ", "│       ", "└       " },
	{ "        ", "╷       ", "│       ", "        " },
	{ "        ", "│       ", "╵       ", "        " },
	{ "╷       ", "│       ", "        ", "        " },
	{ "┌       ", "╵       ", "        ", "        " }
};

void init_colors(void)
{
	start_color();
	use_default_colors();

	init_pair(CP_DEFAULT, COLOR_WHITE, -1);
	init_pair(CP_FRAME, COLOR_WHITE, -1);
	init_pair(CP_ACCENT, COLOR_WHITE, -1);
	init_pair(CP_INVERT, COLOR_BLACK, COLOR_WHITE);
	init_pair(CP_WARN, COLOR_WHITE, COLOR_RED);
	init_pair(CP_DIM, COLOR_BLACK, -1);
	init_pair(CP_METER_ON, COLOR_WHITE, -1);
	init_pair(CP_METER_OFF, COLOR_BLACK, -1);
}

void draw_spinner(int y, int x)
{
	int frame = ui_render_cycle % 25;
	attron(COLOR_PAIR(CP_ACCENT) | A_BOLD);
	for (int i = 0; i < 4; i++) {
		mvprintw(y + i, x, "%s", spinner_frames[frame][i]);
	}
	attroff(COLOR_PAIR(CP_ACCENT) | A_BOLD);
}

void draw_background(void)
{
	attron(COLOR_PAIR(CP_DIM) | A_BOLD);
	for (int y = 0; y < rows; y += 2) {
		for (int x = 0; x < cols; x += 4) {
			if ((x + y + ui_render_cycle) % 10 == 0) {
				attron(A_REVERSE);
				mvaddstr(y, x, "+");
				attroff(A_REVERSE);
			} else {
				mvaddstr(y, x, "+");
			}
		}
	}
	attroff(COLOR_PAIR(CP_DIM) | A_BOLD);

	attron(COLOR_PAIR(CP_INVERT));
	mvhline(0, 0, ' ', cols);
	mvprintw(0, 1, " OVERSEER MONIT ");

	char time_str[64];
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	strftime(time_str, sizeof(time_str), "%H:%M:%S", t);
	mvprintw(0, cols - 10, " %s ", time_str);
	attroff(COLOR_PAIR(CP_INVERT));

	attron(COLOR_PAIR(CP_FRAME) | A_DIM);
	mvhline(rows - 1, 0, ACS_HLINE, cols);

	int wave = ui_render_cycle % 3;
	const char *dots = (wave == 0) ? ".  " : (wave == 1) ? " . " : "  .";
	mvprintw(rows - 1, 2, " CPU: 1%% %s MEM: 124MB %s NET: IDLE ", dots, dots);
	attroff(COLOR_PAIR(CP_FRAME) | A_DIM);
}

void draw_btop_box(int y, int x, int h, int w, const char *title)
{
	attron(COLOR_PAIR(CP_FRAME));
	mvaddstr(y, x, "╭");
	mvaddstr(y, x + w - 1, "╮");
	mvaddstr(y + h - 1, x, "╰");
	mvaddstr(y + h - 1, x + w - 1, "╯");

	mvhline(y, x + 1, ACS_HLINE, w - 2);
	mvhline(y + h - 1, x + 1, ACS_HLINE, w - 2);
	mvvline(y + 1, x, ACS_VLINE, h - 2);
	mvvline(y + 1, x + w - 1, ACS_VLINE, h - 2);
	attroff(COLOR_PAIR(CP_FRAME));

	if (title) {
		attron(COLOR_PAIR(CP_ACCENT) | A_BOLD);
		mvprintw(y, x + 2, " %s ", title);
		attroff(COLOR_PAIR(CP_ACCENT) | A_BOLD);
	}
}

void draw_meter(int y, int x, int w, int percent)
{
	int bar_width = w - 2;
	int fill = (bar_width * percent) / 100;

	attron(COLOR_PAIR(CP_FRAME) | A_DIM);
	mvaddch(y, x, '[');
	mvaddch(y, x + w - 1, ']');
	attroff(COLOR_PAIR(CP_FRAME) | A_DIM);

	for (int i = 0; i < bar_width; i++) {
		if (i < fill) {
			attron(COLOR_PAIR(CP_METER_ON) | A_BOLD);
			mvaddstr(y, x + 1 + i, "/");
			attroff(COLOR_PAIR(CP_METER_ON) | A_BOLD);
		} else {
			attron(COLOR_PAIR(CP_METER_OFF) | A_BOLD);
			mvaddstr(y, x + 1 + i, "-");
			attroff(COLOR_PAIR(CP_METER_OFF) | A_BOLD);
		}
	}
}

void draw_button_btop(int y, int x, int w, const char *text, bool active)
{
	bool hover = (event.y >= y && event.y < y + 3 && event.x >= x && event.x < x + w);
	int color = active ? CP_INVERT : CP_FRAME;

	if (hover) color = CP_INVERT;

	attron(COLOR_PAIR(color));
	mvhline(y, x, ' ', w);
	mvhline(y + 1, x, ' ', w);
	mvhline(y + 2, x, ' ', w);

	if (!hover && !active) {
		attroff(COLOR_PAIR(color));
		attron(COLOR_PAIR(CP_FRAME));
		mvaddch(y, x, ACS_ULCORNER);
		mvaddch(y, x + w - 1, ACS_URCORNER);
		mvaddch(y + 2, x, ACS_LLCORNER);
		mvaddch(y + 2, x + w - 1, ACS_LRCORNER);
		mvhline(y, x + 1, ACS_HLINE, w - 2);
		mvhline(y + 2, x + 1, ACS_HLINE, w - 2);
		mvvline(y + 1, x, ACS_VLINE, 1);
		mvvline(y + 1, x + w - 1, ACS_VLINE, 1);
		attroff(COLOR_PAIR(CP_FRAME));
		attron(COLOR_PAIR(CP_DEFAULT) | A_BOLD);
	} else {
		attron(A_BOLD);
	}

	int text_len = strlen(text);
	int pad = (w - text_len) / 2;
	mvprintw(y + 1, x + pad, "%s", text);
	attroff(COLOR_PAIR(color) | A_BOLD);
}

void draw_server_table(void)
{
	if (scan_in_progress) return;
	int start_y = target_row_start + 2;

	pthread_mutex_lock(&list_mutex);
	int safe_server_count = server_count;

	if (safe_server_count > MAX_SERVERS)
		safe_server_count = MAX_SERVERS;

	attron(COLOR_PAIR(CP_DIM));
	mvprintw(start_y, target_cols_start + 2, "   %-6s %-20s %-8s %-10s ", 
		"ID", "IP ADDRESS", "PORT", "STATUS");

	attroff(COLOR_PAIR(CP_DIM));
	mvhline(start_y + 1, target_cols_start + 1, ACS_HLINE, 
		target_cols_end - target_cols_start - 2);

	for (int i = 0; i < safe_server_count; i++)
	{
		int row_y = start_y + 2 + i;
		if (row_y >= target_row_end - 1) break;

		bool hover = (event.y == row_y && event.x > target_cols_start 
			&& event.x < target_cols_end);

		if (hover)
		{
			attron(COLOR_PAIR(CP_INVERT));
			mvhline(row_y, target_cols_start + 1, ' ', target_cols_end - target_cols_start - 2);
			mvprintw(row_y, target_cols_start + 2, " > %04d   ", server_list[i].server_id);
			printw("%-20s", server_list[i].ip);
			printw(" %-8d %-10s ", server_list[i].port, "ONLINE");
			attroff(COLOR_PAIR(CP_INVERT));
		}
		else
		{
			attron(COLOR_PAIR(CP_DEFAULT));
			if (i % 2 != 0)
				attron(A_DIM);
			mvprintw(row_y, target_cols_start + 2, "   %04d   ", server_list[i].server_id);
			printw("%-20s", server_list[i].ip);
			printw(" %-8d %-10s ", server_list[i].port, "ONLINE");
			if (i % 2 != 0)
				attroff(A_DIM);
			attroff(COLOR_PAIR(CP_DEFAULT));
		}
	}
	
	pthread_mutex_unlock(&list_mutex);
}

void popup_input_btop(void)
{
	int w = 50, h = 8;
	int y = rows / 2 - h / 2;
	int x = cols / 2 - w / 2;

	safe_popup_dimensions(&w, &h, &x, &y);
	attron(COLOR_PAIR(CP_DEFAULT));

	for (int i = 0; i < h; i++) {
		mvhline(y + i, x, ' ', w);
	}

	draw_btop_box(y, x, h, w, "SECURE TRANSMISSION");
	mvprintw(y + 2, x + 2, "ENTER PAYLOAD:");

	attron(A_REVERSE);
	mvhline(y + 4, x + 2, ' ', w - 4);
	attroff(A_REVERSE);

	echo();
	curs_set(1);

	char buf[INPUT_BUFFER_SIZE] = {0};
	move(y + 4, x + 2);
	timeout(-1);

	int max_visible = w - 4 - 1;
	if (max_visible < 0) max_visible = 0;

	safe_getnstr(buf, sizeof(buf), max_visible);
	timeout(10);
	noecho();
	curs_set(0);

	if (strlen(buf) > 0) {
		attron(COLOR_PAIR(CP_INVERT) | A_BLINK);
		mvprintw(y + 6, x + w / 2 - 5, " SENDING ");

		attroff(COLOR_PAIR(CP_INVERT) | A_BLINK);
		refresh();

		core_send_message(current_server.ip, current_server.port, buf);
		usleep(300000);
	}

	attroff(COLOR_PAIR(CP_DEFAULT));
}

int safe_getnstr(char *buf, size_t buf_size, int max_chars)
{
	if (buf == NULL) return -2;

	int limit = (max_chars < (int)buf_size - 1) ? max_chars : (int)buf_size - 1;

	if (limit <= 0) return -1;
	getnstr(buf, limit);

	buf[buf_size - 1] = '\0';
	return 0;
}

void safe_popup_dimensions(int *w, int *h, int *x, int *y)
{
	*w = (*w < 30) ? 30 : *w;
	*h = (*h < 6) ? 6 : *h;

	if (*w > cols - 4) *w = cols - 4;
	if (*h > rows - 4) *h = rows - 4;

	*y = (rows - *h) / 2;
	*x = (cols - *w) / 2;

	if (*y < 0) *y = 0;
	if (*x < 0) *x = 0;

	if (*y + *h > rows) *y = rows - *h;
	if (*x + *w > cols) *x = cols - *w;
}

void on_upload_progress(size_t sent, size_t total, double speed_mbps)
{
	int w = 60, h = 12;
	int y = rows / 2 - h / 2;
	int x = cols / 2 - w / 2;

	int pct = (int)((sent * 100) / total);

	attron(COLOR_PAIR(CP_DEFAULT));
	for (int i = 0; i < h; i++) {
		mvhline(y + i, x, ' ', w);
	}
	draw_btop_box(y, x, h, w, "UPLOADING FILE");

	mvprintw(y + 2, x + 2, "Transferred: %zu / %zu bytes", sent, total);
	mvprintw(y + 3, x + 2, "Speed: %.2f MB/s", speed_mbps);

	draw_meter(y + 5, x + 2, w - 4, pct);

	int spinner_x = x + w / 2 - 4;
	draw_spinner(y + 7, spinner_x);

	attron(A_BOLD);
	mvprintw(y + 7, x + 2, " %d%% COMPLETE ", pct);
	attroff(A_BOLD);

	attroff(COLOR_PAIR(CP_DEFAULT));
	refresh();
}

void popup_file_upload(void)
{
	int w = 50, h = 8;
	int y = rows / 2 - h / 2;
	int x = cols / 2 - w / 2;

	char input_buf[FILEPATH_BUFFER_SIZE] = {0};
	char safe_path[FILEPATH_BUFFER_SIZE] = {0};

	safe_popup_dimensions(&w, &h, &x, &y);
	attron(COLOR_PAIR(CP_DEFAULT));

	for (int i = 0; i < h; i++) {
		mvhline(y + i, x, ' ', w);
	}

	draw_btop_box(y, x, h, w, "FILE UPLOAD");
	mvprintw(y + 2, x + 2, "FILE PATH (relative to %s):", UPLOAD_BASE_DIR);

	attron(A_REVERSE);
	mvhline(y + 4, x + 2, ' ', w - 4);
	attroff(A_REVERSE);

	echo();
	curs_set(1);
	move(y + 4, x + 2);
	timeout(-1);

	int max_visible = w - 4 - 1;
	safe_getnstr(input_buf, sizeof(input_buf), max_visible);

	timeout(10);
	noecho();
	curs_set(0);

	if (strlen(input_buf) > 0) {
		if (!is_path_safe(input_buf, UPLOAD_BASE_DIR)) {
			attron(COLOR_PAIR(CP_DEFAULT));

			for (int i = 0; i < h; i++) {
				mvhline(y + i, x, ' ', w);
			}

			draw_btop_box(y, x, h, w, "SECURITY ERROR");
			attron(COLOR_PAIR(CP_WARN) | A_BOLD);

			mvprintw(y + 3, x + 2, " INVALID FILE PATH ");
			mvprintw(y + 5, x + 2, " PATH TRAVERSAL DETECTED ");
			attroff(COLOR_PAIR(CP_WARN) | A_BOLD);

			refresh();
			usleep(2000000);
			attroff(COLOR_PAIR(CP_DEFAULT));
			return;
		}

		if (!resolve_safe_path(input_buf, safe_path, sizeof(safe_path), UPLOAD_BASE_DIR)) {
			attron(COLOR_PAIR(CP_DEFAULT));

			for (int i = 0; i < h; i++) {
				mvhline(y + i, x, ' ', w);
			}

			draw_btop_box(y, x, h, w, "ERROR");
			attron(COLOR_PAIR(CP_WARN) | A_BOLD);

			mvprintw(y + 3, x + 2, " CANNOT RESOLVE PATH ");
			attroff(COLOR_PAIR(CP_WARN) | A_BOLD);

			refresh();
			usleep(2000000);
			attroff(COLOR_PAIR(CP_DEFAULT));
			return;
		}

		struct stat st;

		if (stat(safe_path, &st) != 0 || !S_ISREG(st.st_mode)) {
			attron(COLOR_PAIR(CP_DEFAULT));

			for (int i = 0; i < h; i++) {
				mvhline(y + i, x, ' ', w);
			}

			draw_btop_box(y, x, h, w, "ERROR");
			attron(COLOR_PAIR(CP_WARN) | A_BOLD);

			mvprintw(y + 3, x + 2, " FILE NOT FOUND ");
			mvprintw(y + 5, x + 2, " or not a regular file ");
			attroff(COLOR_PAIR(CP_WARN) | A_BOLD);

			refresh();
			usleep(2000000);
			attroff(COLOR_PAIR(CP_DEFAULT));
			return;
		}

		int res = core_upload_file(current_server.ip, current_server.port,
					   safe_path, on_upload_progress);

		attron(COLOR_PAIR(CP_DEFAULT));

		for (int i = 0; i < h; i++) {
			mvhline(y + i, x, ' ', w);
		}

		draw_btop_box(y, x, h, w, "STATUS");

		if (res == 0) {
			attron(COLOR_PAIR(CP_INVERT));
			mvprintw(y + 3, x + w / 2 - 8, " UPLOAD COMPLETE ");
			attroff(COLOR_PAIR(CP_INVERT));
		} else {
			attron(COLOR_PAIR(CP_WARN) | A_BOLD);
			mvprintw(y + 3, x + w / 2 - 6, " UPLOAD FAILED ");
			attroff(COLOR_PAIR(CP_WARN) | A_BOLD);
		}

		refresh();
		usleep(1000000);
	}

	attroff(COLOR_PAIR(CP_DEFAULT));
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
			    last_click_x > target_cols_start &&
			    last_click_x < target_cols_end) {
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
			draw_btop_box(cy, cx, h, w, "HANDSHAKE");

			draw_spinner(cy + 2, cx + w / 2 - 4);

			attron(A_BLINK);
			mvprintw(cy + 6, cx + 2, " ESTABLISHING LINK... ");
			attroff(A_BLINK);
			attroff(COLOR_PAIR(CP_DEFAULT));
			refresh();

			usleep(500000);

			if (core_connect(current_server.ip, current_server.port) == 0) {
				connected_to_server = true;
			} else {
				attron(COLOR_PAIR(CP_WARN) | A_BOLD);
				mvprintw(cy + 6, cx + 2, " CONNECTION REFUSED   ");
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
		int btn_disc_y = target_row_start + 16;

		if (last_click_y >= btn_msg_y && last_click_y < btn_msg_y + 3 &&
		    last_click_x >= btn_start_x && last_click_x < btn_start_x + btn_w) {
			popup_input_btop();
		}

		if (last_click_y >= btn_file_y && last_click_y < btn_file_y + 3 &&
		    last_click_x >= btn_start_x && last_click_x < btn_start_x + btn_w) {
			popup_file_upload();
		}

		if (last_click_y >= btn_disc_y && last_click_y < btn_disc_y + 3 &&
		    last_click_x >= btn_start_x && last_click_x < btn_start_x + btn_w) {
			connected_to_server = false;
		}
	}
}

int send_message_safely(const char* message)
{
	if (!connected_to_server) return -1;
	safe_buffer_t safe_buf;

	if (core_init_safe_buffer(&safe_buf, 1024) != 0)
		return -1;

	if (core_set_safe_buffer(&safe_buf, message, strlen(message)) != 0)
	{
		core_destroy_safe_buffer(&safe_buf);
		return -1;
	}

	bool is_valid;

	if (core_validate_server_state(current_server.ip, current_server.port, &is_valid) != 0 || !is_valid)
	{
		core_destroy_safe_buffer(&safe_buf);
		return -1;
	}

	int result = core_send_message_atomic(current_server.ip, current_server.port, &safe_buf);
	core_destroy_safe_buffer(&safe_buf);
	return result;
}

int upload_file_safely(const char* filepath)
{
	if (!connected_to_server) return -1;
	safe_buffer_t path_buf;

	if (core_init_safe_buffer(&path_buf, PATH_MAX) != 0)
		return -1;

	if (core_set_safe_buffer(&path_buf, filepath, strlen(filepath)) != 0)
	{
		core_destroy_safe_buffer(&path_buf);
		return -1;
	}

	bool is_valid;

	if (core_validate_server_state(current_server.ip, current_server.port, &is_valid) != 0 || !is_valid)
	{
		core_destroy_safe_buffer(&path_buf);
		return -1;
	}

	int result = core_upload_file_atomic(current_server.ip, current_server.port, &path_buf,
					     [](size_t sent, size_t total, double speed)
					     {
						     mvprintw(target_row_start + 10, target_cols_start + 3,
							      "Uploading: %zu/%zu bytes (%.2f MB/s)", sent, total, speed);
						     refresh();
					     });

	core_destroy_safe_buffer(&path_buf);
	return result;
}

int execute_operation_atomic(const char* operation_type, const char* data)
{
	safe_buffer_t input_buf;
	if (core_init_safe_buffer(&input_buf, 1024) != 0)
		return -1;

	if (core_set_safe_buffer(&input_buf, data, strlen(data)) != 0)
	{
		core_destroy_safe_buffer(&input_buf);
		return -1;
	}

	bool server_valid = false;
	int validation_result = core_validate_server_state(
	    current_server.ip,
	    current_server.port,
	    &server_valid);

	if (validation_result != 0 || !server_valid || 
	    !connected_to_server ||
	    strlen(data) == 0)
	{

		core_destroy_safe_buffer(&input_buf);
		return -1;
	}

	int operation_result = -1;

	if (strcmp(operation_type, "message") == 0)
	{
		operation_result = core_send_message_atomic(
		    current_server.ip,
		    current_server.port,
		    &input_buf);
	}
	else if (strcmp(operation_type, "file") == 0)
	{
		operation_result = core_upload_file_atomic(
		    current_server.ip,
		    current_server.port,
		    &input_buf,
		    NULL);
	}

	core_destroy_safe_buffer(&input_buf);
	return operation_result;
}