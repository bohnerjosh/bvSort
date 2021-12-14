#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "minHeap.h"
#include "stage_1_sort.h"
#include "structs.h"

/*
    This program sorts large data sets. Stage 1 is the beginning of the sort. 
        It takes an input file, reads DEF_IO_BYTES bytes of it at a time, sorts that data,
        via a multithreaded merge sort and places that sorted data in a temporary file.
        Once the entire input file is read and sorted, stage 2 begins. 

    Stage 2 opens each tempoarary data file and reads SLICE_BYTES bytes from each file.
        This data is placed in a buffer dedicatd to each file. Then, the program takes 
        a number from each buffer, assignes that number an ID to tell which buffer it
        came from, and places it in a min heap. The program then loops by removing the 
        smallest item from the min heap, adds it to the output array, and grabs the next
        integer from the buffer with the same ID (the same buffer the original number in
        the heap came from). If any of the input buffers empty, more data is fetched from 
        temporary files until they are all empty. Every time the output buffer fills up, it 
        gets written to the output file. This process repeats until all the temporary input 
        buffers empty. Then stage 2 terminates and the process completes.  
*/
// Defines
// FD - always means file descriptor

#define SZ 42500000 // size of prep and main arrays
#define DEF_IO_BYTES 42500000*4 // byte amout of main and prep
#define OUTSIZE 8000000
#define OUT_BYTES 8000000*4
#define SLICE_SIZE 400000
#define SLICE_BYTES 400000*4


// Main arrays as pointers to them
unsigned int *mainBuf = NULL; // pointer to buffer to pass to threads to sort data
unsigned int *prepBuf = NULL; // pointer to buffer for main thread to read and write sorted data

// thread array pointers - each points to a quarter slice of MAINBUF for C's built in qsort
unsigned int *t_buf1 = NULL; 
unsigned int *t_buf2 = NULL;
unsigned int *t_buf3 = NULL;
unsigned int *t_buf4 = NULL;

// misc pointers for tracking the pivot and "highest" elements of each subslice
unsigned int *pivotPtr = NULL;
unsigned int *highestPtr = NULL;
unsigned int *LpivotPtr = NULL;
unsigned int *LhighestPtr = NULL;
unsigned int *RpivotPtr = NULL;
unsigned int *RhighestPtr = NULL;

// over variables
short maininFD = 0; // holds FD of input file
short mainoutFD = 0; // holds FD of output file
short stage1 = 1;  // flags for telling the stage of the sort we are at
short stage2 = 0;
int prepArrSize = 0; // Size (actual size - indexable) of prep array
int mainArrSize = 0; // Size (actual size - indexable) of main array
short firstRun = 1; // flag to mark the first pass of stage 1 for loop
short lastRun = 0; // flag to mark the last pass of stage 1 for loop

// global structs
tempSortedData tsData; // contains useful FD, name, etc info for temp files
sliceholder sliceh; // contains pointers to slice structs

void swapArrays() {
    // this function swaps the pointers to the main and prep arrays

    // swap prep and main
    unsigned int *tmp = mainBuf;
    mainBuf = prepBuf;
    prepBuf = tmp;

    // swap the sizes
    int itmp = mainArrSize;
    mainArrSize = prepArrSize;
    prepArrSize = itmp;

    // reset other pointers
    pivotPtr = NULL;
    highestPtr = NULL;
    LhighestPtr = NULL;
    LpivotPtr = NULL;
    RhighestPtr = NULL;
    RpivotPtr = NULL;
}

short setUpFile() {
    // this function sets up a temporary file for intermidiary sorted data
    // along with data structures to track each one

    short index = tsData.fileIndex;

    // make a file name (casting ints to chars in C is interesting)
    char *tmp = (char *) malloc(sizeof(short));
    sprintf(tmp, "%d", index);

    // create the temp file - with correct permissions 
    short fd = open(tmp, O_WRONLY|O_CREAT, S_IRWXU);
    
    // store the size and name
    tsData.sizes[index] = prepArrSize;
    tsData.name[index] = tmp;

    // return the FD
    return fd;
}

void firstRead() {
    // this function is called on startup to fill the temp array with data 
    // from input file
    int bytes = read(maininFD, prepBuf, DEF_IO_BYTES);
    prepArrSize = bytes / 4;
}

void lastWrite() {
    // upon the end of presorting, this function writes the data in the prep array 
    // to a temporary file
    short fd = setUpFile(); // get a FD
    write(fd, prepBuf, prepArrSize*4);
    tsData.fileIndex++;
}

void ArrayIO() {
    // this function handles primary I/O between disk and the prep array to be sorted
    // this happens asynchronously while the main buf is sorting
    short fd = setUpFile(); // get a FD
    // first we write
    write(fd, prepBuf, prepArrSize*4);
    tsData.fileIndex++;
    
    // then we read
    int bytes = read(maininFD, prepBuf, DEF_IO_BYTES);
    prepArrSize = bytes / 4;
    // if the number of bytes read is less than the array size, we are almost done
    if (bytes < DEF_IO_BYTES) {
        lastRun = 1;
    }
}

void* t_qsort(void* arg) {
    // this function sorts a quarter slice of the input array with qsort
    // multithreaded

    qsortData* data = (qsortData *) arg; // cast the input (defined in structs.h)
    qsort(data->arr, data->nmemb, sizeof(unsigned int), cmpInt);
    free(data);
}

void* man_qsort_t(void* arg) {
    // multithreaded manual quicksort function to partition arrays into quarters
    preSort* data = (preSort *) arg; // cast the input (defined in structs.h)
    
    // partition so all the numbers are divided by a pivot
    int pi = partition_t(data->arr, data->low, data->high, data->type);
    int diff = 0;
    // assign poiner to beginning of each quarter slice of the array based on 
    // which side of the initial half it was in
    if (data->type == 1) {
        diff = LhighestPtr - (LpivotPtr + 1);
        t_buf2 = LhighestPtr - diff;
    }
    else {
        diff = RhighestPtr - (RpivotPtr + 1);
        t_buf4 = RhighestPtr - diff;
    }
}

void man_qsort(unsigned int *arr, int low, int high) {
    // manual quicksort function that splits the sort array into halves
    // then it calls 2 threads to split into quarters 
    // Once the threads finish partitioning the function exits
    
    // partition so all the numbers are divided by a pivot (pi)
    int pi = partition(arr, low, high);
   
    // calculate beginnings of thread arrays for later
    int diff = highestPtr - (pivotPtr + 1); 
    t_buf1 = arr;    
    t_buf3 = highestPtr - diff;

    // initialize stage 2 of manual quicksort (multithreaded)

    // pointers to hold important array information
    preSort* right = (preSort *) malloc(sizeof(preSort));
    preSort* left = (preSort *) malloc(sizeof(preSort));

    // presort holds: 
    left->arr = t_buf1; // pointer to array
    left->low = low; // 'low' - lowest index of array
    left->high = pi-1;  // 'high' - highest index of array
    left->type = 1; // type - for indicating which pointers to set
                        // once in thread function
    
    right->arr = t_buf1;
    right->low = pi+1;
    right->high = high;
    right->type = 2;

    pthread_t thread[2];

    // here we go!
    pthread_create(&(thread[0]), NULL, man_qsort_t, left);
    pthread_create(&(thread[1]), NULL, man_qsort_t, right);
    
    // wait for the threads to finish
    pthread_join(thread[0], NULL);
    pthread_join(thread[1], NULL);

    // free the dynamic stuff
    free(left);
    free(right);

    return;
}

short isDone(short ID) {
    // returns a flag stating whether we are done reading from
    // the file associated with the ID
    return (sliceh.data[ID])->done;
}

short fillSlice(short ID) {
    // this function takes a ID representing the index
    // to a stored FD in tsData and a slice in slices
    // and fills the array with data from that file
    slice *s = sliceh.data[ID]; // get the slice
    short fd = tsData.fd[ID]; // get the FD
    int bytes = 0;
    bytes = read(fd, s->arr, SLICE_BYTES); // READ
    if (bytes == 0) {
        // if we read nothing, we hit the end of the file
        // and we are done
        close(fd);
        s->size = -1;
        return -1;
    
    }
    // otherwise we set the size and return true
    s->size = bytes / 4;
    s->readCount++;
    s->index = 0;
    return 1;
}

unsigned int getNumFromSlice(short ID) {
    // This function grabs a single unsigned int from a slice   
    // matching the ID of the given arg
    slice *s = sliceh.data[ID]; // get the slice matching the ID
    int tmp = s->index; // get its index
    unsigned int num = s->arr[tmp]; // get an int from it and increment
    s->index++;
    if (s->index == s->size) { 
        // if the index is equal to size, fill the array with new data
        short result = fillSlice(ID);
        if (result == -1) s->done = 1; // if the slice returns a flag saying
                                        // it is empty, return -1
    }
    return num; // the int
}

void initStage2(mheap *h) {

    // this function initializes the data structures and pointers needed to 
    // combine the presorted data back into one output file

    // init vars
    short fd = 0; // for FDs 
    int bytes = 0;
    // iterate through tsData's array of filenames
    for (int i=0; i<tsData.fileIndex; i++) {
        // open file by name
        fd = open(tsData.name[i], O_RDONLY);
        
        // save its FD
        tsData.fd[i] = fd;

        // make slice and assign its vars
        slice *s = (slice *) malloc(sizeof(slice));
        s->ID = i; // ID to match to index of name, size, and slice
        s->index = 0; // index of int to grab
        s->done = 0; // flag for whether we are done reading from the file
        s->readCount = 1; // number of times we have read from this file
        sliceh.data[i] = s; // store the slice in a global struct
        bytes = read(fd, s->arr, SLICE_BYTES);
        s->size = bytes / 4; // the size of it

        // make a node and add it to the min heap
        Node *node = (Node *) malloc(sizeof(Node));
        node->num = getNumFromSlice(s->ID);
        node->ID = i;
        insert(h, node);
    }
}

void writeOutput(int index, unsigned int *outBuf) {
    // this function write the output of the outBuf to the output file
    if (index < OUTSIZE)
        write(mainoutFD, outBuf, index*4);
    else 
        write(mainoutFD, outBuf, OUT_BYTES);
}

int main(int argc, char** argv) {
    tsData.fileIndex = 0; // init
    if (argc > 1) {
        // open input
        maininFD = open(argv[1], O_RDONLY);
    }

    else {
        return 1;
    }

    // initialize mainbuf and prepbuff, and other vars
    mainBuf = (unsigned int *) malloc(sizeof(unsigned int) * SZ);
    prepBuf = (unsigned int *) malloc(sizeof(unsigned int) * SZ);
    // offsets for multithreaded array sorting
    int mainDiff = 0;
    int Ldiff = 0;
    int Rdiff = 0;

    // read and fill the prep array, and swap it to primary array
    firstRead();
    swapArrays();

    // this is true until all pre-sorting is done
    while(stage1) {
        // call the first manual quicksort function
        man_qsort(mainBuf, 0, mainArrSize-1);

        // now that the data in the array is partitioned, we can initialize
        // threads to do a multithreaded quicksort using qsort

        // pointers to hold important array data to pass to threads
        qsortData* t1 = (qsortData *) malloc(sizeof(qsortData));
        qsortData* t2 = (qsortData *) malloc(sizeof(qsortData));
        qsortData* t3 = (qsortData *) malloc(sizeof(qsortData));
        qsortData* t4 = (qsortData *) malloc(sizeof(qsortData));

        // re-calculate diffs (offsets between low and high for each partition)
        mainDiff = highestPtr - (pivotPtr + 1);
        Ldiff = LhighestPtr - (LpivotPtr + 1);
        Rdiff = RhighestPtr - (RpivotPtr + 1);
        
        t1->arr = t_buf1; // pointer to beginning of array to sort
        t1->nmemb = LpivotPtr-t_buf1; // total number of ints to sort


        t2->arr = t_buf2;
        t2->nmemb = Ldiff+1;
        
        t3->arr = t_buf3;
        t3->nmemb = mainDiff+1;
        
        t4->arr = t_buf4;
        t4->nmemb = Rdiff+1;

        pthread_t thread[4];
        pthread_create(&(thread[0]), NULL, t_qsort, t1);
        pthread_create(&(thread[1]), NULL, t_qsort, t2);
        pthread_create(&(thread[2]), NULL, t_qsort, t3);
        pthread_create(&(thread[3]), NULL, t_qsort, t4);
        // intermidiary section
        // if last iteration, write once and exit
        if (lastRun) {
            lastWrite();
            stage1 = 0;
        }
        // otherwise do normal I/O
        else if (!firstRun && !lastRun) {
            ArrayIO();
        }
        // if first iteration, read into prep buff again without writing
        if (firstRun) {
            firstRead();
            firstRun = 0;
        }

        // wait for threads to finish
        for (int i=0; i<4; i++) {
            pthread_join(thread[i], NULL);
        }
        
        // SWAP
        swapArrays();
    }
    lastWrite();
    // stage 2 
    stage2 = 1; // stage 2 begins
    free(mainBuf); // free the main and temp buffs since we don't need them anymore
    free(prepBuf);

    // open the output file
    mainoutFD = open(argv[2], O_WRONLY | O_APPEND | O_CREAT, S_IRWXU);

    // initialize stage 2 vars
    unsigned int *outBuf = (unsigned int *) malloc(sizeof(unsigned int)* OUTSIZE);
    Node *node = NULL; // node pointer for holding the output of the minheap
    int index = 0; // for holding the index of the output array
    short ID = 0; // for holding the ID of Node
    short done = 0; // Tells stage2 loop it is done after writing last bit of data to output file
    unsigned int tmp = 0; // holds the int from the Node
    mheap* heap = CreateHeap(); // MIN HEAP
    initStage2(heap); // init function
    while(stage2) {
        while (index != OUTSIZE) {
            // get a node from the heap
            node = getMin(heap);
            if (node == NULL) {
                done = 1;
                break;
            }
            ID = node->ID;

            // put it in the out array
            outBuf[index] = node->num;
            index++;
            // Assign the node a new value if the slice isn't empty
            if (!isDone(ID)) {
                //tmp = peekTop(heap);
                node->num = getNumFromSlice(ID);
            
                // add the node to the heap
                insert(heap, node);
            }
            else {
                // if it is empty, free it
                free(node);
                free(sliceh.data[ID]);
            }
        }
        // write the output to disk
        writeOutput(index, outBuf);
        index = 0;
        if (done) stage2 = 0;
    }
    // free remaining variables
    for (int i=0; i<tsData.fileIndex; i++) {
        unlink(tsData.name[i]); // delete the temp files.
        free(tsData.name[i]);
    }
    free(outBuf);
    return 0;
}
