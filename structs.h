#ifndef STRUCT_H
#define STRUCT_H

#define SLICE_SIZE_STRUCT 4000000

struct tempSortedData {
    short fileIndex; // index of the next nuber we can assign as a filename
    short fd[64];  // keeps track of all the temp file descriptors
    int sizes[64]; // keeps track of the size of each file
    char* name[64]; // keeps track of each filename

} typedef tempSortedData;

struct qsortData {
    unsigned int *arr; // pointer to the array we are sorting in threaded qsort 
    int nmemb; // number of items are are sorting
    
} typedef qsortData;

struct preSort {
    unsigned int *arr; // pointer to the array we are partitioning
    int low; // index of lowest element in array
    int high; // index of highest element in array
    short type; // for keeping track of which part of the array we are sorting 
                   // so we can set the correct pointers
} typedef preSort;

// nodes hold data and are stored in the min heap
struct node {
    unsigned int num; // holds the unsigned int from the array
    short ID; // ID representing the temp file the num came from
} typedef Node;

// slice holds partially sorted data from temp file, and has a few
// flags
struct Slice {
    unsigned int arr[SLICE_SIZE_STRUCT]; // array to hold data
    short ID; // ID representing the temp file the num came from
    int size; // real size of array (indexable)
    int index; // index for the array that points to the next
                    // int to grab
    short done; // flag to signify whether we are done reading from the 
                // file
    short readCount; // number of times we have read from the file
} typedef slice;

struct sliceHolder {
    slice* data[64]; // array holding pointers to slices

} typedef sliceholder;


#endif
