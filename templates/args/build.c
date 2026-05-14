/* GURD Build Tool - <https://github.com/Thepigcat76/gurd> */

#include "gurd.h"

#define COMPILER "gcc"
#define DEBUGGER "gdb"
#define STANDARD "c23"
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

  // Pass all additional args after '--args'
  int args_arg_idx = args_contains(argc, argv, "--args");
  // Use argument to determine whether to compile in debug mode and run with
  // debugger, also ensures that '--debug' was passed to gurd, not the program
  int debug_arg_idx = args_contains(argc, argv, "--debug");
  bool debug = debug_arg_idx != -1 && debug_arg_idx < args_arg_idx;

  // The compiler to use
  cmd_appendf(&cmd, COMPILER);
  // Flags
  cmd_appendf(&cmd, "-std=%s", STANDARD);
  if (debug) {
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
    char args[1024] = {'\0'};

    if (args_arg_idx != -1 && args_arg_idx + 1 < argc) {
      for (int i = args_arg_idx + 1; i < argc; i++) {
        strcat(args, argv[i]);
        if (i + 1 < argc) {
          strcat(args, " ");
        }
      }
    }

    // Run using debugger (gdb in this case)
    if (debug) {
      systemf(DEBUGGER " --args ./" OUT_NAME " %s", args);
    } else {
      systemf("./" OUT_NAME);
    }
  }
}