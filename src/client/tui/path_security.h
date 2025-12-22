#ifndef PATH_SECURITY_H
#define PATH_SECURITY_H

#include <stddef.h>
#include <stdbool.h>

bool is_path_safe(const char *path, const char *allowed_base);
char *resolve_safe_path(const char *input_path, char *resolved_buffer, size_t buffer_size, const char *allowed_base);

#endif