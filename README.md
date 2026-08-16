# Gurd

A simple single header c build tool for linux

Run install.sh to install the tool

Look at the 'templates' dir for examples

# How to use

# Build Script

The core component of gurd is the [gurd.sh](./gurd) script that will look for a build.c file in the current directory or the directory specified with **--dir**. It then compiles build.c, outputs it to the .gurd directory and runs it. The build file can then either run the program directly as a unit build or compile it. Args passed to [gurd.sh](./gurd) are passed on to the build file (except for --dir if provided)

# Utility Header

Gurd also comes with the header [gurd.h](./gurd.h) that includes a variety of utility functions like a cmd builder, string formatting, file and directory helpers.
