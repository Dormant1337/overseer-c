#ifndef INTERFACE_H
#define INTERFACE_H

#include <ncurses.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>

#define CP_DEFAULT 1
#define CP_FRAME 2
#define CP_ACCENT 3
#define CP_INVERT 4
#define CP_WARN 5
#define CP_DIM 6
#define CP_METER_ON 7
#define CP_METER_OFF 8

// Render (render.c)
void init_colors(void);
void draw_background(void);

// Components (components.c)
void draw_spinner(int y, int x);
void draw_btop_box(int y, int x, int h, int w, const char *title);
void draw_meter(int y, int x, int w, int percent);
void draw_button_btop(int y, int x, int w, const char *text, bool active);
void draw_server_table(void);

// Popups (popups.c)
void popup_input_btop(void);
void popup_file_upload(void);
void popup_execute_cmd(void);
void popup_show_output(const char *title, const char *content);
void on_upload_progress(size_t sent, size_t total, double speed_mbps);

// Input (input.c)
void handle_input_btop(pthread_t * thread_ptr);
int safe_getnstr(char *buf, size_t buf_size, int max_chars);
void safe_popup_dimensions(int *w, int *h, int *x, int *y);

// Path Security (path_security.c - existing)
bool is_path_safe(const char *path, const char *allowed_base);
char *resolve_safe_path(const char *input_path, char *resolved_buffer,
			size_t buffer_size, const char *allowed_base);

// System/API (likely elsewhere, but declared here in original)
int send_message_safely(const char *message);
int upload_file_safely(const char *filepath);
int execute_operation_atomic(const char *operation_type, const char *data);

#endif
