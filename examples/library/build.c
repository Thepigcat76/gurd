// To build the project, compile this file with a compiler of your choice and
// run the compiled executable. The project also requires the gurd header, which
// can be found at <https://github.com/Thepigcat76/gurd/blob/main/gurd.h>

#include "gurd.h"
#include <stdlib.h>
#include <unistd.h>

#define COMPILER "clang"
#define STANDARD "gnu23"
#define DEBUG true

#define PROJECT_NAME "gurd-prototype"

// Out name for test builds
#define OUT_NAME "build/" PROJECT_NAME
// Out name for library builds
#define LIB_OUT_NAME "build/" PROJECT_NAME ".a"

// The project directory containing header files
#define INCLUDE_DIR "./include/"
// The directory where header files should be moved to after installation
#define INSTALL_INCLUDE_DIR "/usr/include/" PROJECT_NAME "/"

// The destination where the library should be moved to (includes the library
// name)
#define INSTALL_LIB_DEST "/use/lib/lib" PROJECT_NAME ".a"

// Test file, that is included when running the project,
// but ignored when building the library
#define TEST_FILE "test.c"

static Cmd cmd = {0};

static void visit_lib_entry(struct file_entry entry) {
  if (strcmp(entry.file_ext, "c") != 0 || strcmp(entry.name, TEST_FILE) == 0 ||
      entry.name[0] == '_')
    return;

  Cmd lib_compile_cmd = {0};

  cmd_appendf(&lib_compile_cmd, COMPILER);
  // Flags
  cmd_appendf(&lib_compile_cmd, "-g");
  cmd_appendf(&lib_compile_cmd, "-c");
  cmd_appendf(&lib_compile_cmd, "-std=%s", STANDARD);
  // Output location
  cmd_appendf(&lib_compile_cmd, "-o");

  cmd_appendf(&lib_compile_cmd, "./build/%s.o", entry.name);

  cmd_appendf(&lib_compile_cmd, "%s", entry.path);

  cmd_execute(&lib_compile_cmd);

  cmd_appendf(&cmd, "./build/%s.o", entry.name);

  printf("Compiled %s\n", entry.path);
}

static void visit_entry(struct file_entry entry) {
  if (strcmp(entry.file_ext, "c") != 0 || entry.name[0] == '_')
    return;
  cmd_appendf(&cmd, "%s", entry.path);
}

static void lib_install(void);

int main(int argc, char **argv) {
  // Remove old build files
  remove_dir_recursive("build", false);

  size_t lib_arg_idx = args_contains(argc, argv, "--lib");
  bool build_lib = lib_arg_idx != -1;
  bool install_lib = arg_eq(argc, argv, lib_arg_idx + 1, "install");

  if (build_lib) {
    // Make sure out path exists
    ensure_parent_dirs(LIB_OUT_NAME, 0755);

    cmd_appendf(&cmd, "ar rcs " LIB_OUT_NAME);

    walk_dir("src", visit_lib_entry);

    cmd_execute(&cmd);

    if (install_lib) {
      lib_install();
    }
  } else { // The compiler to use
    // Make sure out path exists
    ensure_parent_dirs(OUT_NAME, 0755);

    cmd_appendf(&cmd, COMPILER);
    // Flags
    cmd_appendf(&cmd, "-g");
    cmd_appendf(&cmd, "-std=%s", STANDARD);
    // Output location
    cmd_appendf(&cmd, "-o");
    cmd_appendf(&cmd, OUT_NAME);

    // Adding src files
    walk_dir("src", visit_entry);

    // Run the command
    cmd_execute(&cmd);

    if (arg_eq(argc, argv, 1, "r") || arg_eq(argc, argv, 1, "run")) {
      systemf("./%s", OUT_NAME);
    }
  }
}

static void copy_lib_header_file(struct file_entry file) {
  char dest_file_buf[256];
  sprintf(dest_file_buf, INSTALL_INCLUDE_DIR "/%s", file.name,
          file.file_ext);
  copy_file(file.path, dest_file_buf);
}

static void lib_install(void) {
  if (geteuid() != 0) {
    fprintf(stderr, "Please run the install step with sudo.\n");
    exit(EXIT_FAILURE);
  }

  make_dirs(INSTALL_INCLUDE_DIR, 0755);

  walk_dir(INCLUDE_DIR, copy_lib_header_file);

  if (copy_file(LIB_OUT_NAME, INSTALL_LIB_DEST) != 0) {
    perror("failed to move lib");
  }
}
