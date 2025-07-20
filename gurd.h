#pragma once

#include <dirent.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define ARRAY(...) {__VA_ARGS__, NULL}

#define STR_CMP_OR(str, ...)                                                   \
  _internal_str_cmp_or(str, (char *[128]){__VA_ARGS__})

static bool _internal_str_cmp_or(char *base_str, char **strs) {
  for (int i = 0; strs[i] != NULL; i++) {
    char *str = strs[i];
    if (strcmp(base_str, str) == 0) {
      return true;
    }
  }
  return false;
}

static char _internal_src_files_buf[4096];
static char _internal_libs_buf[4096];
static char _internal_flags_buf[1024];
static char _internal_build_name_buf[256];
static char _internal_cmd_buf[8192];
static char _internal_fmt_buf[8192];

static char *str_fmt(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vsnprintf(_internal_fmt_buf, 4096, fmt, args);
  va_end(args);
  return _internal_fmt_buf;
}

static char *collect_src_files(char *directory) {
  struct dirent *entry;
  DIR *dp = opendir(directory);
  if (dp == NULL) {
    perror("opendir");
    exit(1);
  }

  while ((entry = readdir(dp)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    if (entry->d_type == DT_DIR) {
      char dir_buf[256];
      strcpy(dir_buf, str_fmt("%s/%s/", directory, entry->d_name));
      collect_src_files(dir_buf);
      continue;
    }

    const char *ext;
    const char *dot = strrchr(entry->d_name, '.');
    if (dot != NULL) {
      ext = dot + 1;
      if (ext && strcmp(ext, "c") == 0) {
        strcat(_internal_src_files_buf,
               str_fmt("%s/%s ", directory, entry->d_name));
      }
    }
  }

  closedir(dp);

  return _internal_src_files_buf;
}

static char *link_libs(char **libraries) {
  for (int i = 0; libraries[i] != NULL; i++) {
    strcat(_internal_libs_buf, str_fmt("-l%s ", libraries[i]));
  }
  return _internal_libs_buf;
}

static char *build_name(char *name, int target);

static char *build_flags(void *opts);

static char *build_compiler(char *default_compiler, int target);

static void build_dir(char *dirname) { mkdir(dirname, 0755); }

static void compile(const char *cmd_fmt, ...) {
  va_list args;
  va_start(args, cmd_fmt);
  vsnprintf(_internal_cmd_buf, 4096, cmd_fmt, args);
  va_end(args);

  system(_internal_cmd_buf);
}

static void run(char *build_dir, char *build_name) {
  char run_buf[512];
  snprintf(run_buf, 512, "./%s/%s", build_dir, build_name);
  system(run_buf);
}

static void dbg(char *build_dir, char *build_name, bool dbg) {
  if (!dbg) {
    fprintf(stderr, "Debug needs to be enabled in the build options in order to be able to debug the program\n");
    exit(1);
  }
  char dbg_buf[512];
  snprintf(dbg_buf, 512, "gdb ./%s/%s", build_dir, build_name);
  system(dbg_buf);
}
