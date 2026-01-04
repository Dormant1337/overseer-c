#define _XOPEN_SOURCE_EXTENDED
#include "../globals.h"
#include "../system/api.h"
#include "interface.h"
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define INPUT_BUFFER_SIZE 256
#define OUTPUT_BUFFER_SIZE 8192

void popup_show_output(const char *title, const char *content)
{
	int w = cols - 16;
	int h = rows - 8;
	if (w < 40)
		w = 40;
	if (h < 12)
		h = 12;

	int y = rows / 2 - h / 2;
	int x = cols / 2 - w / 2;

	char *text_copy = strdup(content);
	if (!text_copy)
		return;

	char *lines[4096];
	int line_count = 0;
	char *p = text_copy;
	lines[line_count++] = p;

	while (*p && line_count < 4096) {
		if (*p == '\n') {
			*p = '\0';
			lines[line_count++] = p + 1;
		}
		p++;
	}

	int scroll = 0;
	bool running = true;
	MEVENT local_event;

	while (running) {
		int view_h = h - 5;
		int max_scroll = line_count - view_h;
		if (max_scroll < 0)
			max_scroll = 0;

		if (scroll < 0)
			scroll = 0;
		if (scroll > max_scroll)
			scroll = max_scroll;

		attron(COLOR_PAIR(CP_DEFAULT));
		for (int i = 0; i < h; i++) {
			mvhline(y + i, x, ' ', w);
		}

		draw_btop_box(y, x, h, w, title);

		for (int i = 0; i < view_h; i++) {
			int idx = scroll + i;
			if (idx >= line_count)
				break;

			char line_buf[512];
			strncpy(line_buf, lines[idx], w - 4);
			line_buf[w - 4] = '\0';
			mvprintw(y + 2 + i, x + 2, "%s", line_buf);
		}

		if (max_scroll > 0) {
			int sb_h = view_h;
			int sb_y = y + 2;
			int thumb_pos = (scroll * (sb_h - 1)) / max_scroll;

			attron(COLOR_PAIR(CP_DIM));
			mvvline(sb_y, x + w - 1, ACS_VLINE, sb_h);
			attroff(COLOR_PAIR(CP_DIM));

			attron(COLOR_PAIR(CP_ACCENT));
			mvaddch(sb_y + thumb_pos, x + w - 1, ACS_CKBOARD);
			attroff(COLOR_PAIR(CP_ACCENT));
		}

		int btn_w = 12;
		int btn_x = x + w / 2 - btn_w / 2;
		int btn_y = y + h - 2;

		attron(COLOR_PAIR(CP_INVERT));
		mvprintw(btn_y, btn_x, " [ CLOSE ] ");
		attroff(COLOR_PAIR(CP_INVERT));

		refresh();

		int ch = getch();
		if (ch == 'q' || ch == 27) {
			running = false;
		} else if (ch == KEY_UP) {
			scroll--;
		} else if (ch == KEY_DOWN) {
			scroll++;
		} else if (ch == KEY_PPAGE) {
			scroll -= view_h;
		} else if (ch == KEY_NPAGE) {
			scroll += view_h;
		} else if (ch == KEY_MOUSE) {
			if (getmouse(&local_event) == OK) {
				if (local_event.y == btn_y
				    && local_event.x >= btn_x
				    && local_event.x < btn_x + btn_w
				    && (local_event.
					bstate & (BUTTON1_CLICKED |
						  BUTTON1_PRESSED))) {
					running = false;
				}
#ifdef BUTTON4_PRESSED
				if (local_event.bstate & BUTTON4_PRESSED)
					scroll--;
#endif
#ifdef BUTTON5_PRESSED
				if (local_event.bstate & BUTTON5_PRESSED)
					scroll++;
#endif
			}
		}
	}

	free(text_copy);
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

	char buf[INPUT_BUFFER_SIZE] = { 0 };
	move(y + 4, x + 2);
	timeout(-1);

	int max_visible = w - 4 - 1;
	if (max_visible < 0)
		max_visible = 0;

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

void popup_execute_cmd(void)
{
	int w = 60, h = 8;
	int y = rows / 2 - h / 2;
	int x = cols / 2 - w / 2;

	safe_popup_dimensions(&w, &h, &x, &y);
	attron(COLOR_PAIR(CP_DEFAULT));

	for (int i = 0; i < h; i++) {
		mvhline(y + i, x, ' ', w);
	}

	draw_btop_box(y, x, h, w, "REMOTE EXECUTION");
	mvprintw(y + 2, x + 2, "ENTER COMMAND:");

	attron(A_REVERSE);
	mvhline(y + 4, x + 2, ' ', w - 4);
	attroff(A_REVERSE);

	echo();
	curs_set(1);

	char buf[INPUT_BUFFER_SIZE] = { 0 };
	move(y + 4, x + 2);
	timeout(-1);

	int max_visible = w - 4 - 1;
	safe_getnstr(buf, sizeof(buf), max_visible);
	timeout(10);
	noecho();
	curs_set(0);

	if (strlen(buf) > 0) {
		attron(COLOR_PAIR(CP_INVERT) | A_BLINK);
		mvprintw(y + 6, x + w / 2 - 6, " EXECUTING ");
		attroff(COLOR_PAIR(CP_INVERT) | A_BLINK);
		refresh();

		char *output = malloc(OUTPUT_BUFFER_SIZE);
		if (output) {
			if (core_execute_command
			    (current_server.ip, current_server.port, buf,
			     output, OUTPUT_BUFFER_SIZE) == 0) {
				popup_show_output("STDOUT/STDERR", output);
			} else {
				popup_show_output("ERROR",
						  "Failed to execute command or receive response.");
			}
			free(output);
		}
	}
	attroff(COLOR_PAIR(CP_DEFAULT));
}
