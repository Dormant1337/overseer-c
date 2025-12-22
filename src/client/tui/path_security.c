#define _XOPEN_SOURCE 700
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <libgen.h>
#include <errno.h>
#include "interface.h"
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#ifdef PATH_MAX
#define MAX_PATH_LEN PATH_MAX
#else
#define MAX_PATH_LEN 4096
#endif

/* common dangerous path patterns */
static const char *dangerous_patterns[] = {
    "../",
    "..\\",
    "/..",
    "\\..",
    "//",
    "\\\\",
    "~",
    NULL};

/* Check if path contains dangerous traversal sequences. */
static bool contains_traversal(const char *path)
{
        if (!path || path[0] == '\0') return true;
        if (path[0] == '/') return true;

        for (int i = 0; dangerous_patterns[i] != NULL; i++)
        {
                if (strstr(path, dangerous_patterns[i]) != NULL)
                        return true;
        }

        const char *basename = strrchr(path, '/');
        if (!basename) basename = strrchr(path, '\\');
        if (!basename) basename = path;

        if (basename[0] == '.' && basename[1] == '.')
                return true;

        return false;
}

/* Normalize path separators following the Unix-style. */
static void normalize_path(char *path)
{
        if (!path) return;

        for (char *p = path; *p; p++)
        {
                if (*p == '\\')
                        *p = '/';
        }
}

bool is_path_safe(const char* path, const char* allowed_base)
{
        if (!path || path[0] == '\0')
                return false;

        char path_copy[MAX_PATH_LEN];

        if (strlen(path) >= MAX_PATH_LEN - 1)
                return false;

        strncpy(path_copy, path, MAX_PATH_LEN - 1);
        path_copy[MAX_PATH_LEN - 1] = '\0';
        normalize_path(path_copy);

        if (contains_traversal(path_copy))
                return false;

        if (allowed_base && allowed_base[0] != '\0')
        {
                char resolved_path[MAX_PATH_LEN];
                char base_path[MAX_PATH_LEN];

                if (getcwd(base_path, sizeof(base_path)) == NULL)
                        return false;

                if (allowed_base[0] != '/')
                {
                        snprintf(resolved_path, sizeof(resolved_path), "%s/%s",
                                 base_path, allowed_base);
                }
                else
                        strncpy(resolved_path, allowed_base, sizeof(resolved_path) - 1);
                char full_user_path[MAX_PATH_LEN];

                if (realpath(path_copy, full_user_path) == NULL)
                {
                        snprintf(full_user_path, sizeof(full_user_path), "%s/%s",
                                 resolved_path, path_copy);
                }

                if (strncmp(full_user_path, resolved_path, strlen(resolved_path)) != 0)
                        return false;
        }

        struct stat st;

        if (stat(path_copy, &st) == 0)
        {
                if (!S_ISREG(st.st_mode))
                        return false;

                if (access(path_copy, R_OK) != 0)
                        return false;
        }

        return true;
}

char *resolve_safe_path(const char* input_path, char* resolved_buffer, size_t buffer_size, const char* allowed_base)
{
        if (!input_path || !resolved_buffer || buffer_size < MAX_PATH_LEN)
                return NULL;

        if (!is_path_safe(input_path, allowed_base))
                return NULL;

        char path_copy[MAX_PATH_LEN];
        strncpy(path_copy, input_path, sizeof(path_copy) - 1);

        path_copy[sizeof(path_copy) - 1] = '\0';
        normalize_path(path_copy);

        if (allowed_base && allowed_base[0] != '\0')
        {
                char base_copy[MAX_PATH_LEN];
                strncpy(base_copy, allowed_base, sizeof(base_copy) - 1);
                base_copy[sizeof(base_copy) - 1] = '\0';

                normalize_path(base_copy);
                snprintf(resolved_buffer, buffer_size, "%s/%s", base_copy, path_copy);
        }
        else
        {
                strncpy(resolved_buffer, path_copy, buffer_size - 1);
                resolved_buffer[buffer_size - 1] = '\0';
        }

        char* real = realpath(resolved_buffer, NULL);

        if (real)
        {
                if (allowed_base && allowed_base[0] != '\0')
                {
                        char real_base[MAX_PATH_LEN];
                        snprintf(real_base, sizeof(real_base), "%s/", allowed_base);
                        normalize_path(real_base);

                        if (strncmp(real, real_base, strlen(real_base)) != 0)
                        {
                                free(real);
                                return NULL;
                        }
                }

                strncpy(resolved_buffer, real, buffer_size - 1);
                resolved_buffer[buffer_size - 1] = '\0';
                free(real);
        }

        return resolved_buffer;
}