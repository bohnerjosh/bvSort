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

struct node {
    unsigned int num;
    short ID;
} typedef Node;

struct Slice {
    unsigned int arr[SLICE_SIZE_STRUCT];
    short ID;
    int size;
    int index;
    short done;
    short readCount;
} typedef slice;

struct sliceHolder {
    slice* data[64];

} typedef sliceholder;


#endif
