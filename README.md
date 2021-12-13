# bvSort

This is a repository for my final project for CMSC 432 Operating Systems course (Fall 2021).

This program's purpose is to sort large data sets under the restriction of 1 GB of memory and 25 GB of disk space.

The output is sorted in ascending order.

# Dependencies

This assumes you have a native C compiler installed, such as `gcc`. 

# Installation 

Clone the repository.

```git clone https://github.com/bohnerjosh/bvSort/```

# Compiling & Running

This example uses `gcc` for the compiler

To compile this program:

```
gcc bvSort.c -lpthread -o bvSort
```

To run this program:
```
./bvSort <inputfile> <outputfile>
```
Where:
- `<inputfile>`: A file containing 32-bit unsigned integers.
- `<outputfile>`: A file created by the program that sort the input in ascending order (small to large). Contains Binary (non-ASCII/UTF8) data.

Note: you can use the 4_gb_file_maker included to generate an file for the input of this program under the data generator directory.
