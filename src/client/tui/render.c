#define _XOPEN_SOURCE_EXTENDED
#include "../globals.h"
#include "interface.h"
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

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

	if (connected_to_server) {
		mvprintw(rows - 1, 2,
			 " CPU: %.0f%% %s MEM: %zu/%zuMB %s NET: ACTIVE ",
			 current_server.cpu_usage, dots,
			 current_server.mem_used, current_server.mem_total,
			 dots);
	} else {
		mvprintw(rows - 1, 2, " CPU: --- %s MEM: --- %s NET: IDLE ",
			 dots, dots);
	}
	attroff(COLOR_PAIR(CP_FRAME) | A_DIM);
}
