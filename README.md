# ToyVM

ToyVM is a custom assembly compiler and a simple virtual machine/interpreter to execute the compiled code. It runs on Windows, Linux, and OSX. It supports linking c/c++ dynamic libraries through a simple plugin interface.

As of right now, there are two main programs in the repo.

* tcom
* tvm

## tcom

Is the assembly compiler.

### tcom Usage

```txt
tcom <options> <input file>

   options:
      -h show this message.
      -o output file.
      -l link library.
      -d disable full path when reporting errors.
      -m print the module path and exit.
```

* -l Links a shared library into the file.
* -m Displays the location of the shared library folder.
* -d Is used in the tests to prevent full path names from being reported, which would cause them to fail.

## tvm

tvm runs the executable file generated from tcom.

### tvm Usage

```txt
tvm <options> <program_path>

   options:
      -h display this message.
      -t display execution time.
      -m print the module path and exit.
```

### Current instructions

Documentation can be found [here.](Codes.md)

## Building

Building with CMAKE.

```txt
mkdir Build
cd Build
cmake ..
```

Testing and Installing.

```txt
mkdir Build
cd Build
cmake -DToyVM_INSTALL_PATH=<some install directory> -DBUILD_TEST=ON ..
```

By default the executables are copied into ${CMAKE_BINARY_DIR}/bin and tested from that directory.
