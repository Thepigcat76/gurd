#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif

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

#ifdef _WIN32
static char *join_path(const char *directory, const char *name) {
  size_t directory_len = strlen(directory);
  size_t name_len = strlen(name);

  int needs_separator = directory_len > 0 &&
                        directory[directory_len - 1] != '\\' &&
                        directory[directory_len - 1] != '/';

  size_t size = directory_len + needs_separator + name_len + 1;
  char *result = malloc(size);

  if (result == NULL) {
    return NULL;
  }

  snprintf(result, size, "%s%s%s", directory, needs_separator ? "\\" : "",
           name);

  return result;
}
#endif

static void walk_dir(const char *path, void (*visit_func)(struct file_entry)) {
#ifdef _WIN32
  char *search_pattern = join_path(path, "*");

  if (search_pattern == NULL) {
    fprintf(stderr, "Out of memory\n");
    return;
  }

  WIN32_FIND_DATAA data;
  HANDLE find_handle = FindFirstFileA(search_pattern, &data);

  free(search_pattern);

  if (find_handle == INVALID_HANDLE_VALUE) {
    fprintf(stderr, "FindFirstFile failed for \"%s\": error %lu\n", path,
            GetLastError());
    return;
  }

  do {
    const char *name = data.cFileName;

    /*
     * Windows can return these special entries.
     */
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
      continue;
    }

    char *entry_path = join_path(path, name);

    if (entry_path == NULL) {
      fprintf(stderr, "Out of memory\n");
      break;
    }

    if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      walk_dir(entry_path, visit_func);
      free(entry_path);
      continue;
    }

    const char *dot = strrchr(name, '.');
    const char *file_ext = NULL;

    if (dot != NULL && dot != name) {
      file_ext = dot + 1;
    }

    struct file_entry entry = {
        .path = entry_path,
        .name = name,
        .file_ext = file_ext,
    };

    /*
     * visit_func must use/copy these values before it returns.
     */
    visit_func(entry);

    free(entry_path);

  } while (FindNextFileA(find_handle, &data));

  DWORD error = GetLastError();

  /*
   * Use FindClose, not CloseHandle, for search handles.
   */
  FindClose(find_handle);

  if (error != ERROR_NO_MORE_FILES) {
    fprintf(stderr, "FindNextFile failed for \"%s\": error %lu\n", path, error);
  }
#else
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
#endif
}

static int remove_dir_recursive(const char *path, bool remove_self) {
#ifdef _WIN32
  char *search_pattern = join_path(path, "*");

  if (search_pattern == NULL) {
    fprintf(stderr, "Out of memory\n");
    return -1;
  }

  WIN32_FIND_DATAA data;
  HANDLE find_handle = FindFirstFileA(search_pattern, &data);

  if (find_handle == INVALID_HANDLE_VALUE) {
    return -1;
  }

  int rc = 0;

  do {
    const char *name = data.cFileName;

    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
      continue;
    }

    char *child = join_path(path, name);

    if (child == NULL) {
      SetLastError(ERROR_FILENAME_EXCED_RANGE);
      rc = -1;
      break;
    }

    DWORD attributes = data.dwFileAttributes;

    if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
      /*
       * Do not recurse into junctions or directory symlinks.
       * Treat them as directory entries and remove the link itself.
       */
      if (attributes & FILE_ATTRIBUTE_REPARSE_POINT) {
        if (!RemoveDirectoryA(child)) {
          rc = -1;
          break;
        }
      } else {
        if (remove_dir_recursive(child, 1) != 0) {
          rc = -1;
          break;
        }
      }
    } else {
      /*
       * DeleteFile fails for read-only files, so make the file
       * normal first.
       */
      if (attributes & FILE_ATTRIBUTE_READONLY) {
        if (!SetFileAttributesA(child, attributes & ~FILE_ATTRIBUTE_READONLY)) {
          rc = -1;
          break;
        }
      }

      if (!DeleteFileA(child)) {
        rc = -1;
        break;
      }
    }

  } while (FindNextFileA(find_handle, &data));

  DWORD find_error = GetLastError();

  FindClose(find_handle);

  /*
   * FindNextFile returns false normally when enumeration is complete.
   */
  if (rc == 0 && find_error != ERROR_NO_MORE_FILES) {
    rc = -1;
  }

  if (rc == 0 && remove_self) {
    if (!RemoveDirectoryA(path)) {
      rc = -1;
    }
  }

  return rc;
#else
  DIR *dir = opendir(path);
  if (!dir)
    return -1;

  int rc = 0;
  struct dirent *ent;

  while ((ent = readdir(dir)) != NULL) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
      continue;

    char child[PATH_MAX];
    if (snprintf(child, sizeof child, "%s/%s", path, ent->d_name) >=
        (int)sizeof child) {
      errno = ENAMETOOLONG;
      rc = -1;
      break;
    }

    struct stat st;
    if (lstat(child, &st) != 0) {
      rc = -1;
      break;
    }

    if (S_ISDIR(st.st_mode)) {
      if (remove_dir_recursive(child, 1) != 0) {
        rc = -1;
        break;
      }
    } else {
      if (unlink(child) != 0) {
        rc = -1;
        break;
      }
    }
  }

  closedir(dir);

  if (rc == 0 && remove_self) {
    if (rmdir(path) != 0)
      rc = -1;
  }

  return rc;
#endif
}

static int ensure_parent_dirs(const char *filepath) {
  #ifdef _WIN32
  #define mkdir(path, ...) _mkdir(path)
  #define PATH_MAX MAX_PATH
  #else
  mode_t mode = 0755;
  #endif
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

  #ifdef _WIN32
  #undef PATH_MAX
  #undef mkdir
  #endif
}

static int copy_file(const char *src, const char *dst) {
  FILE *in = fopen(src, "rb");
  if (!in)
    return -1;

  FILE *out = fopen(dst, "wb");
  if (!out) {
    fclose(in);
    return -1;
  }

  char buffer[8192];
  size_t n;

  while ((n = fread(buffer, 1, sizeof(buffer), in)) > 0) {
    if (fwrite(buffer, 1, n, out) != n) {
      fclose(in);
      fclose(out);
      return -1;
    }
  }

  fclose(in);
  fclose(out);
  return 0;
}

static int make_dirs(const char *directory) {
  #ifdef _WIN32
  #define mkdir(path, ...) _mkdir(path)
  #define PATH_MAX MAX_PATH
  #else
  mode_t mode = 0755;
  #endif
  char tmp[PATH_MAX];
  if (snprintf(tmp, sizeof tmp, "%s", directory) >= (int)sizeof tmp) {
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

  #ifdef _WIN32
  #undef PATH_MAX
  #undef mkdir
  #endif
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((format(printf, 2, 3)))
#endif
static void
cmd_appendf(Cmd *cmd, const char *fmt, ...) {
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

static size_t cmd_sprint(const Cmd *cmd, char *buf, size_t buf_size) {
  size_t pos = 0;
  if (buf != NULL)
    buf[0] = '\0';

  for (size_t i = 0; i < cmd->len; i++) {
    const char *fmt = (i == cmd->len - 1) ? "%s" : "%s ";
    int n = snprintf(buf ? buf + pos : NULL, buf ? (buf_size - pos) : 0, fmt,
                     cmd->buf[i]);
    if (n < 0)
      return pos;
    pos += (size_t)n;
  }
  return pos;
}

static int cmd_execute(Cmd *cmd) {
  if (cmd->buf == NULL)
    return -1;

  size_t cmd_len = cmd_sprint(cmd, NULL, 0);

  char cmd_buf[cmd_len + 1];
  cmd_sprint(cmd, cmd_buf, cmd_len + 1);

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
static int
systemf(const char *fmt, ...) {
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

  va_end(args);

  return system(str);
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

// Returns the index of 'search_arg' or -1, if it cant be found
static int args_contains_len(int argc, char **argv, const char *search_arg,
                             size_t arg_len) {
  for (int i = 0; i < argc; i++) {
    if (strncmp(argv[i], search_arg, arg_len) == 0) {
      return i;
    }
  }
  return -1;
}

static bool arg_eq(int argc, char **argv, size_t idx, const char *arg) {
  return argc > idx && strcmp(argv[idx], arg) == 0;
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((format(printf, 1, 2)))
#endif
static char *
str_fmt_temp(const char *fmt, ...) {
  static char str_fmt_temp_buf[4096] = {[0] = '\0'};
  va_list args;
  va_start(args, fmt);

  vsprintf(str_fmt_temp_buf, fmt, args);
  va_end(args);

  return str_fmt_temp_buf;
}

#define gurd_build(directory, build_file, ...)                                 \
  _internal_gurd_build(directory, build_file,                                  \
                       (const char **)(char *[]){__VA_ARGS__},                 \
                       sizeof((char *[]){__VA_ARGS__}) / sizeof(char *))

static bool _internal_gurd_build(const char *directory, const char *build_file,
                                 const char **args, size_t args_len) {
  char args_buf[4096];

  args_buf[0] = '\0';

  for (size_t i = 0; i < args_len; i++) {
    strcat(args_buf, args[i]);
    strcat(args_buf, " ");
  }

  int exit_code = 0;

  if (build_file != NULL) {
    exit_code = systemf("gurd --dir %s %s %s", directory, build_file, args_buf);
  } else {
    exit_code = systemf("gurd --dir %s %s", directory, args_buf);
  }

#ifdef _WIN32
  return exit_code != -1;
#else
  return !WEXITSTATUS(exit_code);
#endif
}
