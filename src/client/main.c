#define _XOPEN_SOURCE_EXTENDED
#include <ncurses.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include "globals.h"
#include "tui/interface.h"
#include "system/network.h"

pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
struct ServerInfo server_list[MAX_SERVERS];
struct ServerInfo current_server;
volatile int server_count = 0;

bool connected_to_server = false;
bool scan_in_progress = false;
int scan_render_cycle = 0;
struct timeval scan_last_time;
volatile bool beacon_thread_active = false;

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
        struct timeval anim_last_time;
        gettimeofday(&anim_last_time, NULL);

        while (1) {
                getmaxyx(stdscr, rows, cols);
                int ch = getch();

                if (ch == 'q') break;
                if (ch == KEY_MOUSE && getmouse(&event) == OK) {
                        if (event.bstate & (BUTTON1_PRESSED | BUTTON1_CLICKED)) {
                                last_click_x = event.x;
                                last_click_y = event.y;
                                handle_mouse_click(&beacon_thread);
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
                draw_background_grid();

                if (term_too_small) {
                        draw_fancy_box(rows / 2 - 2, cols / 2 - 15, 4, 30, "ERROR");
                        attron(COLOR_PAIR(CP_WARNING));
                        mvprintw(rows / 2, cols / 2 - 9, "TERMINAL TOO SMALL");
                        attroff(COLOR_PAIR(CP_WARNING));
                } else {
                        draw_fancy_box(target_row_start, target_cols_start, box_h, box_w,
                                       connected_to_server ? "SECURE CONNECTION" : "NETWORK SCANNER");

                        if (!connected_to_server) {
                                if (!scan_in_progress) {
                                        draw_server_list_panel();

                                        int btn_w = 20;
                                        int btn_x = target_cols_start + box_w / 2 - btn_w / 2;
                                        int btn_y = target_row_end - 2;

                                        draw_styled_button(btn_y, btn_x, btn_w, "INITIATE SCAN",
                                                           CP_BUTTON, CP_BUTTON_HOVER, false);

                                } else {
                                        attron(COLOR_PAIR(CP_MAIN_BOX));
                                        mvprintw(rows / 2 - 1, cols / 2 - 10, "SCANNING FREQUENCIES...");
                                        attroff(COLOR_PAIR(CP_MAIN_BOX));
                                        draw_progress_bar_fancy(rows / 2, cols / 2 - 20, 40);
                                }
                        } else {
                                attron(COLOR_PAIR(CP_MAIN_BOX) | A_BOLD);
                                mvprintw(target_row_start + 2, target_cols_start + 4, "TARGET ACQUIRED:");
                                attroff(COLOR_PAIR(CP_MAIN_BOX) | A_BOLD);

                                attron(COLOR_PAIR(CP_TITLE));
                                mvprintw(target_row_start + 3, target_cols_start + 4, ">> %s : %d",
                                         current_server.ip, current_server.port);
                                attroff(COLOR_PAIR(CP_TITLE));

                                draw_styled_button(target_row_start + 5, target_cols_start + 4, 22, "TRANSMIT DATA",
                                                   CP_BUTTON, CP_BUTTON_HOVER, false);

                                draw_styled_button(target_row_start + 7, target_cols_start + 4, 22, "TERMINATE LINK",
                                                   CP_WARNING, CP_WARNING, false);

                                draw_fancy_box(target_row_start + 2, target_cols_end - 24, 8, 20, "SIGNAL");
                                attron(COLOR_PAIR(CP_MAIN_BOX));
                                mvprintw(target_row_start + 4, target_cols_end - 20, "STR: |||||||||");
                                mvprintw(target_row_start + 6, target_cols_end - 20, "ENC: AES-256");
                                attroff(COLOR_PAIR(CP_MAIN_BOX));
                        }
                }

                struct timeval now;
                gettimeofday(&now, NULL);

                if (scan_in_progress) {
                        long ms = (now.tv_sec - scan_last_time.tv_sec) * 1000 + (now.tv_usec - scan_last_time.tv_usec) / 1000;
                        if (ms > 150) {
                                scan_render_cycle++;
                                scan_last_time = now;
                                if (scan_render_cycle > 20) {
                                        scan_in_progress = false;
                                        beacon_thread_active = false;
                                        if (beacon_thread) pthread_join(beacon_thread, NULL);
                                }
                        }
                }

                refresh();
        }

        beacon_thread_active = false;
        if (beacon_thread) pthread_join(beacon_thread, NULL);
        endwin();
        printf("\033[?1003l\n");
        return 0;
}