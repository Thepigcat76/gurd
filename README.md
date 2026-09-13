# Gurd

A simple single header c build tool for linux

Run install.sh to install the tool

You can change the install directory by editing [install.sh](./install.sh). By default, /usr/local/bin and /usr/local/include are used.

Look at the 'examples' dir for examples

# How to use

## Build Script

The core component of gurd is the [gurd.sh](./gurd) script that will look for a build.c file in the current directory or the directory specified with **--dir**. It then compiles build.c, outputs it to the .gurd directory and runs it. The build file can then either run the program directly as a unit build or compile it. Args passed to [gurd.sh](./gurd) are passed on to the build file (except for --dir if provided)

You can change the compiler used to compile the build file by editing [gurd.sh](./gurd). By default, gcc is used.

**Important**: If you are building a library make sure to pass in `--lib` (`gurd --lib`)

## Building Gurd Project without Gurd

If you do not want to install gurd, simply drop the header file [gurd.h](./gurd.h) in your project directory or include path and compile the build file using your preferred c compiler.

- Compile build file `clang build.c -o build-file`

- Run build file `./build-file` this will build the program (most programs are output to `./build/`). If you are building a library pass in `--lib`: `./build-file --lib`

## Utility Header

Gurd also comes with the header [gurd.h](./gurd.h) that includes a variety of utility functions like a cmd builder, string formatting, file and directory helpers.
Most build script will require this so either put it in your include directory or in the project directory.
