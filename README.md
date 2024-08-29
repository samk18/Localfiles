# first-module
An example of a multiple file "modular" C++ program

In this case `main.cpp` uses the `average` function defined in `avg.cpp`.  To pull this off we 
include the header file `avg.h` which announces the `average` function and its contract.

To run use `g++ avg.cpp main.cpp` then `./a.out`.
