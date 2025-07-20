/* To build the project, compile this file with a compiler of your choice and run the compiled executable.
 * The project also requires the gurd header, which can be found at <https://github.com/Thepigcat76/gurd/blob/main/gurd.h>
 */

#include "gurd.h"

typedef enum {
  TARGET_LINUX,
  TARGET_WIN,
} BuildTarget;

typedef struct {
  char *compiler;
  bool debug;
  bool release;
  char *std;
  BuildTarget target;
  char *out_dir;
  char *out_name;
  char *libraries[64];
} BuildOptions;

static BuildOptions OPTS = {.compiler = "clang",
                            .debug = true,
                            .release = false,
                            .std = "c23",
                            .target = TARGET_LINUX,
                            .out_dir = "./build/",
                            .out_name = "example",
                            .libraries = ARRAY("m", "pthread")};

int main(int argc, char **argv) {
  char *compiler = build_compiler(OPTS.compiler, OPTS.target);
  char *files = collect_src_files("./src/");
  char *libraries = link_libs(OPTS.libraries);
  char *flags = build_flags(&OPTS);
  char *out_name = build_name(OPTS.out_name, OPTS.target);

  build_dir(OPTS.out_dir);
  compile("%s %s %s %s -o %s%s", OPTS.compiler, files, libraries, flags, OPTS.out_dir, out_name);

  if (argc >= 2) {
    if (STR_CMP_OR(argv[1], "r", "run")) {
      run(OPTS.out_dir, out_name);
      return 0;
    } else if (STR_CMP_OR(argv[1], "d", "dbg")) {
      dbg(OPTS.out_dir, out_name, OPTS.debug);
      return 0;
    } else {
      fprintf(stderr, "[Error]: Invalid first arg: %s\n", argv[1]);
      return 1;
    }
  }
}

static char *build_name(char *default_name, int target) {
  if (target == TARGET_WIN) {
    strcpy(_internal_build_name_buf, str_fmt("%s.exe", default_name));
    return _internal_build_name_buf;
  }
  return default_name;
}

static char *build_compiler(char *default_compiler, int target) {
  if (target == TARGET_WIN) {
    return "x86_64-w64-mingw32-gcc";
  }
  return default_compiler;
}

static char *build_flags(void *_opts) {
  BuildOptions *opts = (BuildOptions *)_opts;
  _internal_flags_buf[0] = '\0';

  if (opts->debug) {
    strcat(_internal_flags_buf, "-g ");
  } else if (opts->release) {
    strcat(_internal_flags_buf, "-O3 ");
  }

  if (opts->std != NULL) {
    strcat(_internal_flags_buf, str_fmt("-std=%s ", opts->std));
  }

  return _internal_flags_buf;
}
