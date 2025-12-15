#define _XOPEN_SOURCE_EXTENDED
#include <ncurses.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>
#include "interface.h"
#include "../globals.h"
#include "../system/network.h"


void init_colors(void)
{
        start_color();
        if (can_change_color() && COLORS >= 8) {
                init_color(COLOR_CYAN, 0, 800, 800);
                init_color(COLOR_BLUE, 0, 0, 200);
        }

        init_pair(CP_BASE, COLOR_CYAN, COLOR_BLACK);
        init_pair(CP_MAIN_BOX, COLOR_WHITE, COLOR_BLUE);
        init_pair(CP_BUTTON, COLOR_BLACK, COLOR_CYAN);
        init_pair(CP_BUTTON_HOVER, COLOR_BLACK, COLOR_WHITE);
        init_pair(CP_SHADOW, COLOR_BLACK, COLOR_BLACK);
        init_pair(CP_TITLE, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(CP_WARNING, COLOR_WHITE, COLOR_RED);
        init_pair(CP_GRID, COLOR_BLUE, COLOR_BLACK);
}

/**
 * Draws a shadow under a box.
 *
 * This function is used to draw a shadow under a box. It is used to give a box a 3D
 * appearance. The shadow is drawn using spaces and is two pixels larger than the
 * box it is drawn under.
 *
 * @param y The y coordinate of the box.
 * @param x The x coordinate of the box.
 * @param h The height of the box.
 * @param w The width of the box.
 */
void draw_shadow(int y, int x, int h, int w)
{
        attron(COLOR_PAIR(CP_SHADOW) | A_BOLD);
        for (int i = 0; i < h; i++) {
                mvprintw(y + i, x, "%*s", w, " ");
        }
        attroff(COLOR_PAIR(CP_SHADOW) | A_BOLD);
}


/**
 * Draws the background grid.
 *
 * This function is used to draw the background grid. The grid is drawn using
 * periods and is used to give the terminal a grid-like appearance.
 */
void draw_background_grid(void)
{
        attron(COLOR_PAIR(CP_GRID) | A_DIM);
        for (int y = 0; y < rows; y += 2) {
                for (int x = 0; x < cols; x += 4) {
                        mvaddch(y, x, '.');
                }
        }
        attroff(COLOR_PAIR(CP_GRID) | A_DIM);

        attron(COLOR_PAIR(CP_BUTTON));
        mvprintw(0, 0, "%*s", cols, " ");
        mvprintw(0, 1, " NETWORK TERMINAL");
        mvprintw(0, cols - 20, " TIME: %ld ", time(NULL) % 10000);
        attroff(COLOR_PAIR(CP_BUTTON));

        attron(COLOR_PAIR(CP_BUTTON));
        mvprintw(rows - 1, 0, "%*s", cols, " ");
        mvprintw(rows - 1, 1, " STATUS: %s ", connected_to_server ? "LINKED" : "IDLE");
        mvprintw(rows - 1, cols - 15, " MOUSE: %03d,%03d ", event.x, event.y);
        attroff(COLOR_PAIR(CP_BUTTON));
}

/**
 * Draws a fancy box with a given title.
 *
 * This function is used to draw a fancy box with a given title. It is used
 * to draw boxes with titles in the terminal.
 *
 * @param y The y coordinate of the box.
 * @param x The x coordinate of the box.
 * @param h The height of the box.
 * @param w The width of the box.
 * @param title The title of the box.
 */
void draw_fancy_box(int y, int x, int h, int w, char *title)
{
        draw_shadow(y + 1, x + 2, h, w);

        attron(COLOR_PAIR(CP_MAIN_BOX));
        for (int i = 0; i < h; i++) {
                mvprintw(y + i, x, "%*s", w, " ");
        }

        mvprintw(y, x, "╔");
        for (int i = 0; i < w - 2; i++) addstr("═");
        addstr("╗");

        for (int i = 1; i < h - 1; i++) {
                mvprintw(y + i, x, "║");
                mvprintw(y + i, x + w - 1, "║");
        }

        mvprintw(y + h - 1, x, "╚");
        for (int i = 0; i < w - 2; i++) addstr("═");
        addstr("╝");

        if (title) {
                int title_len = strlen(title);
                int tx = x + (w / 2) - (title_len / 2) - 2;
                mvprintw(y, tx, "╡ %s ╞", title);
        }
        attroff(COLOR_PAIR(CP_MAIN_BOX));
}

/**
 * Draws a styled button with a given text and size.
 *
 * This function is used to draw a styled button with a given text and size.
 * It is used to draw buttons in the terminal.
 *
 * @param y The y coordinate of the button.
 * @param x The x coordinate of the button.
 * @param w The width of the button.
 * @param text The text of the button.
 * @param pair_normal The color pair for a normal button.
 * @param pair_hover The color pair for a hover button.
 * @param clicked Whether the button is currently clicked.
 */
void draw_styled_button(int y, int x, int w, char *text, int pair_normal, int pair_hover, bool clicked)
{
        draw_shadow(y + 1, x + 2, 1, w);

        bool is_hover = (event.y == y && event.x >= x && event.x < x + w);
        int pair = is_hover ? pair_hover : pair_normal;

        if (clicked && is_hover) attron(A_REVERSE);

        attron(COLOR_PAIR(pair) | A_BOLD);
        int pad = (w - strlen(text)) / 2;
        mvprintw(y, x, "%*s%s%*s", pad, " ", text, w - pad - strlen(text), " ");
        attroff(COLOR_PAIR(pair) | A_BOLD);

        if (clicked && is_hover) attroff(A_REVERSE);
}

/**
 * Draws a progress bar with a fancy box.
 *
 * This function is used to draw a progress bar with a fancy box. It is used
 * to draw progress bars in the terminal.
 *
 * @param y The y coordinate of the progress bar.
 * @param x The x coordinate of the progress bar.
 * @param w The width of the progress bar.
 */
void draw_progress_bar_fancy(int y, int x, int w)
{
        draw_shadow(y + 1, x + 1, 1, w);

        attron(COLOR_PAIR(CP_MAIN_BOX));
        mvprintw(y, x, "[");
        mvprintw(y, x + w - 1, "]");
        attroff(COLOR_PAIR(CP_MAIN_BOX));

        int inner_w = w - 2;
        int filled = (inner_w * scan_render_cycle) / 20;

        attron(COLOR_PAIR(CP_TITLE) | A_BOLD);
        for (int i = 0; i < inner_w; i++) {
                if (i < filled) mvaddstr(y, x + 1 + i, "▓");
                else mvaddstr(y, x + 1 + i, "░");
        }
        attroff(COLOR_PAIR(CP_TITLE) | A_BOLD);
}


/**
 * Draws the server list panel with a fancy box.
 *
 * This function is used to draw the server list panel with a fancy box. It is used
 * to draw the server list panel in the terminal.
 */
void draw_server_list_panel(void)
{
        if (scan_in_progress) return;

        int list_start_y = target_row_start + 2;

        pthread_mutex_lock(&list_mutex);

        attron(COLOR_PAIR(CP_MAIN_BOX) | A_UNDERLINE);
        mvprintw(list_start_y - 1, target_cols_start + 2, "   SERVER IP      |  PORT  |   ID   ");
        attroff(COLOR_PAIR(CP_MAIN_BOX) | A_UNDERLINE);

        for (int i = 0; i < server_count; i++) {
                int y = list_start_y + i;
                if (y >= target_row_end - 2) break;

                bool hover = (event.y == y && event.x >= target_cols_start + 1 && event.x < target_cols_end - 1);

                if (hover) {
                        attron(COLOR_PAIR(CP_BUTTON_HOVER) | A_BOLD);
                        mvprintw(y, target_cols_start + 1, "%*s", target_cols_end - target_cols_start - 2, " ");
                } else {
                        attron(COLOR_PAIR(CP_MAIN_BOX));
                }

                mvprintw(y, target_cols_start + 2, " %-16s | %-6d | #%04d ",
                         server_list[i].ip, server_list[i].port, server_list[i].server_id);

                if (hover) attroff(COLOR_PAIR(CP_BUTTON_HOVER) | A_BOLD);
                else attroff(COLOR_PAIR(CP_MAIN_BOX));
        }
        pthread_mutex_unlock(&list_mutex);
}

/**
 * Pops up an input message box for the user to enter a message payload.
 *
 * This function is used to popup an input message box for the user to enter a message
 * payload. It is used to get a message payload from the user.
 */
void popup_input_message(void)
{
        int w = 50, h = 10;
        int sy = rows / 2 - h / 2, sx = cols / 2 - w / 2;

        draw_fancy_box(sy, sx, h, w, "SECURE COMMS");

        attron(COLOR_PAIR(CP_MAIN_BOX));
        mvprintw(sy + 3, sx + 2, "ENTER MESSAGE PAYLOAD:");

        attron(A_REVERSE);
        mvprintw(sy + 5, sx + 2, "%*s", w - 4, " ");
        attroff(A_REVERSE);
        attroff(COLOR_PAIR(CP_MAIN_BOX));

        echo();
        curs_set(1);
        char buf[128] = {0};
        move(sy + 5, sx + 2);
        timeout(-1);
        getnstr(buf, w - 5);
        timeout(10);
        noecho();
        curs_set(0);

        if (strlen(buf) > 0) {
                attron(COLOR_PAIR(CP_BUTTON) | A_BLINK);
                mvprintw(sy + 7, sx + w / 2 - 5, " SENDING ");
                attroff(COLOR_PAIR(CP_BUTTON) | A_BLINK);
                refresh();
                send_message(current_server.ip, current_server.port, buf);
                usleep(400000);
        }
}

/**
 * Handles a mouse click event. If the user is not connected to a server and not
 * scanning for servers, the function will reset the server list and start scanning
 * for servers when the user clicks on the "SCAN" button.
 */
void handle_mouse_click(pthread_t *thread_ptr)
{
        if (!connected_to_server && !scan_in_progress) {
                int btn_w = 20;
                int btn_x = target_cols_start + (target_cols_end - target_cols_start) / 2 - btn_w / 2;
                int btn_y = target_row_end - 2;

                if (last_click_y == btn_y && last_click_x >= btn_x && last_click_x <= btn_x + btn_w) {
                        pthread_mutex_lock(&list_mutex);
                        server_count = 0;
                        pthread_mutex_unlock(&list_mutex);

                        scan_in_progress = true;
                        scan_render_cycle = 0;
                        gettimeofday(&scan_last_time, NULL);

                        beacon_thread_active = true;
                        if (*thread_ptr) pthread_join(*thread_ptr, NULL);
                        pthread_create(thread_ptr, NULL, beacon_listener, NULL);
                        return;
                }

                int start_y = target_row_start + 2;
                for (int i = 0; i < server_count; i++) {
                        if (last_click_y == start_y + i && last_click_x > target_cols_start && last_click_x < target_cols_end) {
                                current_server = server_list[i];

                                int mw = 30, mh = 5;
                                draw_fancy_box(rows / 2 - mh / 2, cols / 2 - mw / 2, mh, mw, "SYSTEM");
                                attron(COLOR_PAIR(CP_MAIN_BOX));
                                mvprintw(rows / 2, cols / 2 - 6, "CONNECTING...");
                                attroff(COLOR_PAIR(CP_MAIN_BOX));
                                refresh();

                                if (connect_handshake(current_server.ip, current_server.port) == 0) {
                                        connected_to_server = true;
                                } else {
                                        draw_fancy_box(rows / 2 - mh / 2, cols / 2 - mw / 2, mh, mw, "ERROR");
                                        attron(COLOR_PAIR(CP_WARNING));
                                        mvprintw(rows / 2, cols / 2 - 4, " FAILED ");
                                        attroff(COLOR_PAIR(CP_WARNING));
                                        refresh();
                                        usleep(600000);
                                }
                                return;
                        }
                }
        }

        if (connected_to_server) {
                int btn_base_x = target_cols_start + 4;
                int btn_msg_y = target_row_start + 5;
                int btn_disc_y = target_row_start + 7;
                int btn_w = 22;

                if (last_click_y == btn_msg_y && last_click_x >= btn_base_x && last_click_x < btn_base_x + btn_w) {
                        popup_input_message();
                }

                if (last_click_y == btn_disc_y && last_click_x >= btn_base_x && last_click_x < btn_base_x + btn_w) {
                        connected_to_server = false;
                }
        }
}