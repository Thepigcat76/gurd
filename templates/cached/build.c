/* GURD Build Tool - <https://github.com/Thepigcat76/gurd> */

#include "gurd.h"

#define COMPILER "gcc"
#define STANDARD "c23"
#define DEBUG true
#define OUT_NAME "build/gurd-prototype"

#define LIB_MATH "m"

static Cmd cmd = {0};

static void visit_entry(struct file_entry entry) {
  if (entry.file_ext == NULL || strcmp(entry.file_ext, "c") != 0)
    return;

  Cmd compile_cmd = {0};

  cmd_appendf(&compile_cmd, "ccache");
  cmd_appendf(&compile_cmd, COMPILER);

  // Compile source file in object mode
  cmd_appendf(&compile_cmd, "-c");
  cmd_appendf(&compile_cmd, "%s", entry.path);

  // Flags
  cmd_appendf(&compile_cmd, "-std=%s", STANDARD);
  if (DEBUG) {
    cmd_appendf(&compile_cmd, "-g");
  }
  // Output location
  cmd_appendf(&compile_cmd, "-o");

  const char *src_path = strncmp(entry.path, "src/", 3) == 0 ? entry.path + 4 : entry.path;
  cmd_appendf(&compile_cmd, "./build/%s.o", src_path);

  // Ensure output location exists

  char build_path[512];
  sprintf(build_path, "./build/%s.o", src_path);
  ensure_parent_dirs(build_path, 0755);

  // Execute compile command

  if (cmd_execute(&compile_cmd)) {
    exit(1);
  }
}

static void visit_obj_entry(struct file_entry entry) {
  if (entry.file_ext == NULL || strcmp(entry.file_ext, "o") != 0)
    return;

  cmd_appendf(&cmd, "%s", entry.path);
}

int main(int argc, char **argv) {
  // Remove old build files
  remove_dir_recursive("build", false);

  // Compile src files (cached)
  walk_dir("src", visit_entry);

  // Link obj files to binary
  cmd_appendf(&cmd, COMPILER);

  walk_dir("build", visit_obj_entry);

  // Libraries
  cmd_appendf(&cmd, "-l" LIB_MATH);

  cmd_appendf(&cmd, "-o");
  cmd_appendf(&cmd, OUT_NAME);

  // Run the command
  cmd_execute(&cmd);

  // Run program
  if (arg_eq(argc, argv, 1, "r")) {
    systemf("./" OUT_NAME);
  }

}