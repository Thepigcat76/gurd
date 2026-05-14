#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

struct file_entry {
  const char *path;
  const char *name;
  const char *file_ext;
};

typedef struct {
  char **buf;
  size_t len;
  size_t capacity;
} Cmd;

static void walk_dir(const char *path, void (*visit_func)(struct file_entry)) {
  struct dirent *entry;
  DIR *dp = opendir(path);
  if (dp == NULL) {
    perror("opendir");
    fprintf(stderr, "Path: %s\n", path);
    exit(1);
  }

  while ((entry = readdir(dp)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    char dir_buf[256];
    sprintf(dir_buf, "%s/%s", path, entry->d_name);
    if (entry->d_type == DT_DIR) {
      walk_dir(dir_buf, visit_func);
      continue;
    } else {
      const char *dot = strrchr(entry->d_name, '.');
      const char *file_ext = NULL;
      if (dot != NULL) {
        file_ext = dot + 1;
      }
      struct file_entry file_entry = {
          .path = dir_buf,
          .name = entry->d_name,
          .file_ext = file_ext,
      };
      visit_func(file_entry);
    }
  }

  closedir(dp);
}

static int remove_dir_recursive(const char *path, bool remove_self)
{
    DIR *dir = opendir(path);
    if (!dir) return -1;

    int rc = 0;
    struct dirent *ent;

    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;

        char child[PATH_MAX];
        if (snprintf(child, sizeof child, "%s/%s", path, ent->d_name) >= (int)sizeof child) {
            errno = ENAMETOOLONG;
            rc = -1;
            break;
        }

        struct stat st;
        if (lstat(child, &st) != 0) { rc = -1; break; }

        if (S_ISDIR(st.st_mode)) {
            if (remove_dir_recursive(child, 1) != 0) { rc = -1; break; }
        } else {
            if (unlink(child) != 0) { rc = -1; break; }
        }
    }

    closedir(dir);

    if (rc == 0 && remove_self) {
        if (rmdir(path) != 0) rc = -1;
    }

    return rc;
}

static int ensure_parent_dirs(const char *filepath, mode_t mode) {
  char dir[PATH_MAX];
  if (snprintf(dir, sizeof dir, "%s", filepath) >= (int)sizeof dir) {
    errno = ENAMETOOLONG;
    return -1;
  }

  char *slash = strrchr(dir, '/');
  if (!slash)
    return 0; // no directory component
  if (slash == dir)
    return 0; // parent is "/"

  *slash = '\0'; // keep just the directory part
  if (!*dir) {
    errno = EINVAL;
    return -1;
  }

  char tmp[PATH_MAX];
  if (snprintf(tmp, sizeof tmp, "%s", dir) >= (int)sizeof tmp) {
    errno = ENAMETOOLONG;
    return -1;
  }

  size_t len = strlen(tmp);
  if (len > 1 && tmp[len - 1] == '/')
    tmp[len - 1] = '\0';

  for (char *p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = '\0';
      if (mkdir(tmp, mode) != 0 && errno != EEXIST)
        return -1;
      *p = '/';
    }
  }
  if (mkdir(tmp, mode) != 0 && errno != EEXIST)
    return -1;
  return 0;
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((format(printf, 2, 3)))
#endif
static void cmd_appendf(Cmd *cmd, const char *fmt, ...) {
  if (cmd->buf == NULL) {
    cmd->len = 0;
    cmd->capacity = 16;
    cmd->buf = (char **)malloc(cmd->capacity * sizeof(char *));
  }

  va_list args;
  va_start(args, fmt);

  va_list args_copy;
  va_copy(args_copy, args);

  int len = vsnprintf(NULL, 0, fmt, args_copy);
  va_end(args_copy);

  if (len < 0) {
    va_end(args);
    return;
  }

  char *str = (char *)malloc(len + 1);
  vsnprintf(str, len + 1, fmt, args);

  if (cmd->capacity <= cmd->len) {
    cmd->capacity *= 2;
    cmd->buf = (char **)realloc(cmd->buf, cmd->capacity * sizeof(char *));
  }

  cmd->buf[cmd->len++] = str;

  va_end(args);
}

static size_t cmd_fprint(const Cmd *cmd, FILE *f_stream) {
  size_t len = 0;
  for (size_t i = 0; i < cmd->len; i++) {
    len += fprintf(f_stream, i == cmd->len - 1 ? "%s" : "%s ", cmd->buf[i]);
  }
  return len;
}

#define SIZE_MAX 4096

static size_t cmd_sprint(const Cmd *cmd, char *buf) {
  size_t pos = 0;
  if (buf != NULL)
    buf[0] = '\0';

  for (size_t i = 0; i < cmd->len; i++) {
    const char *fmt = (i == cmd->len - 1) ? "%s" : "%s ";
    int n = snprintf(buf ? buf + pos : NULL, buf ? (SIZE_MAX - pos) : 0, fmt,
                     cmd->buf[i]);
    if (n < 0)
      return pos;
    pos += (size_t)n;
  }
  return pos;
}

static int cmd_execute(Cmd *cmd) {
  if (cmd->buf == NULL) return -1;

  size_t cmd_len = cmd_sprint(cmd, NULL);

  char cmd_buf[cmd_len + 1];
  cmd_sprint(cmd, cmd_buf);

  for (size_t i = 0; i < cmd->len; i++) {
    free(cmd->buf[i]);
  }

  free(cmd->buf);

  cmd->buf = NULL;

  return system(cmd_buf);
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((format(printf, 1, 2)))
#endif
static int systemf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  va_list args_copy;
  va_copy(args_copy, args);

  int len = vsnprintf(NULL, 0, fmt, args_copy);
  va_end(args_copy);

  if (len < 0) {
    va_end(args);
    return -1;
  }

  char str[len + 1];
  vsprintf(str, fmt, args);

  return system(str);

  va_end(args);
}

// Returns the index of 'search_arg' or -1, if it cant be found
static int args_contains(int argc, char **argv, const char *search_arg) {
  for (size_t i = 0; i < argc; i++) {
    if (strcmp(argv[i], search_arg) == 0) {
      return i;
    }
  }
  return -1;
}

static bool arg_eq(int argc, char **argv, size_t idx, const char *arg) {
  return argc > idx && strcmp(argv[idx], arg) == 0;
}
