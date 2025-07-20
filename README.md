# Gurd
A simple single header c build tool for linux with cross compilation capabilities for windows (soonTM)

# How to use

Create a build.c file (can have any name, but build.c is gurd convention)

Copy the [standard build file layout](https://github.com/Thepigcat76/gurd/blob/main/example/basic_build.c)

Adjust the build options if needed (out_name, out_dir, compiler, c-std, libraries...)

Compile the build file using your preferred compiler and run the compiled executable.
Passing r/run will run the project and passing d/dbg while run the project with gdb.
You can add your own command line options, just adjust/copy the if statement in the build file.

Finally, it makes sense to automate the compiling and executing of your build file. This can be done using a simple bash script.
