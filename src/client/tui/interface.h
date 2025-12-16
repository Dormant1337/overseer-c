#ifndef INTERFACE_H
#define INTERFACE_H

#include <pthread.h>

#define CP_DEFAULT      1
#define CP_FRAME        2
#define CP_ACCENT       3
#define CP_INVERT       4
#define CP_WARN         5
#define CP_DIM          6
#define CP_METER_ON     7
#define CP_METER_OFF    8

void init_colors(void);
void draw_background(void);
void draw_btop_box(int y, int x, int h, int w, const char *title);
void draw_meter(int y, int x, int w, int percent);
void draw_button_btop(int y, int x, int w, const char *text, bool active);
void draw_server_table(void);
void popup_input_btop(void);
void handle_input_btop(pthread_t *thread_ptr);

#endif