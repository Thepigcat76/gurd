#include "gurd.h"

#define COMPILER "gcc"
#define STANDARD "c23"
#define DEBUG true
#define OUT_NAME "build/test"

static Cmd cmd = {0};

static void visit_entry(struct file_entry entry) {
  cmd_appendf(&cmd, "%s", entry.path);
}

int main() {
  // The compiler to use
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
}
