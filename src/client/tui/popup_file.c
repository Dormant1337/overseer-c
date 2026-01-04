#define _XOPEN_SOURCE_EXTENDED
#include "../globals.h"
#include "../system/api.h"
#include "interface.h"
#include "path_security.h"
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define FILEPATH_BUFFER_SIZE 512

#ifndef UPLOAD_BASE_DIR
#define UPLOAD_BASE_DIR "./uploads"
#endif

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

	char input_buf[FILEPATH_BUFFER_SIZE] = { 0 };
	char safe_path[FILEPATH_BUFFER_SIZE] = { 0 };

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

		if (!resolve_safe_path(input_buf, safe_path, sizeof(safe_path),
				       UPLOAD_BASE_DIR)) {
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

		int res =
		    core_upload_file(current_server.ip, current_server.port,
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
