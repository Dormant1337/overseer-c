#ifndef INTERFACE_H
#define INTERFACE_H

#include <pthread.h>

#define CP_BASE         1
#define CP_MAIN_BOX     2
#define CP_BUTTON       3
#define CP_BUTTON_HOVER 4
#define CP_SHADOW       5
#define CP_TITLE        6
#define CP_WARNING      7
#define CP_GRID         8

void init_colors(void);
void draw_background_grid(void);
void draw_fancy_box(int y, int x, int h, int w, char *title);
void draw_styled_button(int y, int x, int w, char *text, int pair_normal, int pair_hover, bool clicked);
void draw_progress_bar_fancy(int y, int x, int w);
void draw_server_list_panel(void);
void popup_input_message(void);
void handle_mouse_click(pthread_t *thread_ptr);

#endif