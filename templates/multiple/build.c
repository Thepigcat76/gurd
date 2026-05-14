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

  cmd_appendf(&cmd, "%s", entry.path);
}

int main(int argc, char **argv) {
  // Remove old build files
  remove_dir_recursive("build", false);

  // The compiler to use
  cmd_appendf(&cmd, COMPILER);
  // Flags
  cmd_appendf(&cmd, "-std=%s", STANDARD);
  if (DEBUG) {
    cmd_appendf(&cmd, "-g");
  }
  // Output location
  cmd_appendf(&cmd, "-o");
  cmd_appendf(&cmd, OUT_NAME);

  // Adding src files
  walk_dir("src", visit_entry);

  // Libraries
  cmd_appendf(&cmd, "-l" LIB_MATH);

  // Make sure out path exists
  ensure_parent_dirs(OUT_NAME, 0755);

  // Run the command
  cmd_execute(&cmd);

  // Run program
  if (arg_eq(argc, argv, 1, "r")) {
    systemf("./" OUT_NAME);
  }

}
