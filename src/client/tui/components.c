#define _XOPEN_SOURCE_EXTENDED
#include "../globals.h"
#include "interface.h"
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

static const char *spinner_frames[25][4] = {
	{"┌──     ", "        ", "        ", "        "},
	{" ───    ", "        ", "        ", "        "},
	{"  ───   ", "        ", "        ", "        "},
	{"   ───  ", "        ", "        ", "        "},
	{"    ─── ", "        ", "        ", "        "},
	{"     ──┐", "        ", "        ", "        "},
	{"      ─┐", "       ╵", "        ", "        "},
	{"       ┐", "       │", "        ", "        "},
	{"        ", "       │", "       ╵", "        "},
	{"        ", "       ╷", "       │", "        "},
	{"        ", "        ", "       │", "       ╵"},
	{"        ", "        ", "       ╷", "       ┘"},
	{"        ", "        ", "        ", "      ─┘"},
	{"        ", "        ", "        ", "     ──┘"},
	{"        ", "        ", "        ", "    ─── "},
	{"        ", "        ", "        ", "   ───  "},
	{"        ", "        ", "        ", "  ───   "},
	{"        ", "        ", "        ", " ───    "},
	{"        ", "        ", "        ", "└──     "},
	{"        ", "        ", "        ", "╷       "},
	{"        ", "        ", "│       ", "└       "},
	{"        ", "╷       ", "│       ", "        "},
	{"        ", "│       ", "╵       ", "        "},
	{"╷       ", "│       ", "        ", "        "},
	{"┌       ", "╵       ", "        ", "        "}
};

void draw_spinner(int y, int x)
{
	int frame = ui_render_cycle % 25;
	attron(COLOR_PAIR(CP_ACCENT) | A_BOLD);
	for (int i = 0; i < 4; i++) {
		mvprintw(y + i, x, "%s", spinner_frames[frame][i]);
	}
	attroff(COLOR_PAIR(CP_ACCENT) | A_BOLD);
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
	bool hover =
	    (event.y >= y && event.y < y + 3 && event.x >= x
	     && event.x < x + w);
	int color = active ? CP_INVERT : CP_FRAME;

	if (hover)
		color = CP_INVERT;

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
	if (scan_in_progress)
		return;
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

	for (int i = 0; i < safe_server_count; i++) {
		int row_y = start_y + 2 + i;
		if (row_y >= target_row_end - 1)
			break;

		bool hover = (event.y == row_y && event.x > target_cols_start &&
			      event.x < target_cols_end);

		if (hover) {
			attron(COLOR_PAIR(CP_INVERT));
			mvhline(row_y, target_cols_start + 1, ' ',
				target_cols_end - target_cols_start - 2);
			mvprintw(row_y, target_cols_start + 2, " > %04d   ",
				 server_list[i].server_id);
			printw("%-20s", server_list[i].ip);
			printw(" %-8d %-10s ", server_list[i].port, "ONLINE");
			attroff(COLOR_PAIR(CP_INVERT));
		} else {
			attron(COLOR_PAIR(CP_DEFAULT));
			if (i % 2 != 0)
				attron(A_DIM);
			mvprintw(row_y, target_cols_start + 2, "   %04d   ",
				 server_list[i].server_id);
			printw("%-20s", server_list[i].ip);
			printw(" %-8d %-10s ", server_list[i].port, "ONLINE");
			if (i % 2 != 0)
				attroff(A_DIM);
			attroff(COLOR_PAIR(CP_DEFAULT));
		}
	}

	pthread_mutex_unlock(&list_mutex);
}
