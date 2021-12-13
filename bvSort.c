#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "minHeap.h"
#include "stage_1_sort.h"
#include "structs.h"

// Defines
#define SZ 42500000
#define DEF_IO_BYTES 42500000*4
#define BUFTOTAL 4
#define OUTSIZE 8000000
#define OUT_BYTES 8000000*4
#define SLICE_SIZE 400000
#define SLICE_BYTES 400000*4

pthread_mutex_t wait = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t stopwait = PTHREAD_COND_INITIALIZER;

// Main arrays as pointers to them
unsigned int *mainBuf = NULL;
unsigned int *prepBuf = NULL;

// thread array pointers
unsigned int *t_buf1 = NULL;
unsigned int *t_buf2 = NULL;
unsigned int *t_buf3 = NULL;
unsigned int *t_buf4 = NULL;

// misc pointers for tracking the pivot and "highest" elements 
unsigned int *pivotPtr = NULL;
unsigned int *highestPtr = NULL;
unsigned int *LpivotPtr = NULL;
unsigned int *LhighestPtr = NULL;
unsigned int *RpivotPtr = NULL;
unsigned int *RhighestPtr = NULL;

// over variables
int bufTotal = 0;
short maininFD = 0;
short mainoutFD = 0;
short stage1 = 1;
short stage2 = 0;
int prepArrSize = 0;
int mainArrSize = 0;
short firstRun = 1;
short lastRun = 0;
short t_count = 0;

tempSortedData tsData;
sliceholder sliceh;

void swapArrays() {
    printf("Swapping...\n");
    fflush(stdout);
    // swap prep and main
    unsigned int *tmp = mainBuf;
    mainBuf = prepBuf;
    prepBuf = tmp;

    // swap the sizes
    int itmp = mainArrSize;
    mainArrSize = prepArrSize;
    prepArrSize = itmp;

    // reset the pointers
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
    char *tmp = (char *) malloc(sizeof(short));
    sprintf(tmp, "%d", index);
    short fd = open(tmp, O_WRONLY|O_CREAT, S_IRWXU);
    
    // size keeps track of how full each file is
    tsData.sizes[index] = prepArrSize;
    tsData.name[index] = tmp;
    return fd;
}
void* t_qsort(void* arg) {
    t_count = 0;
    // this function sorts a quarter silice of the input array with qsort
    qsortData* data = (qsortData *) arg;
    qsort(data->arr, data->nmemb, sizeof(unsigned int), cmpInt);
    free(data);
    /*
    while(1) {
        if (!pthread_mutex_trylock(&wait)) {
            break;
        }
    }
    t_count += 1;
    if (t_count == 4) {
        pthread_mutex_unlock(&wait);
    }
    else {
        pthread_mutex_unlock(&wait);
        while (t_count != 4) {
            usleep(100000);
        }
    }
    */
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
    short fd = setUpFile();
    write(fd, prepBuf, prepArrSize*4);
    tsData.fileIndex++;
}

void ArrayIO() {
    // this function handles primary I/O between disk and the prep array to be sorted
    printf("Doing array I/O...\n");
    fflush(stdout);
    short fd = setUpFile();
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

void* man_qsort_t(void* arg) {
    // threaded manual quicksort function to partition arrays into quarters
    preSort* data = (preSort *) arg;
    
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
    
    // partition so all the numbers are divided by a pivot
    int pi = partition(arr, low, high);
   
    // calculate beginnings of thread arrays for later
    int diff = highestPtr - (pivotPtr + 1); 
    t_buf1 = arr;    
    t_buf3 = highestPtr - diff;

    // initialize stage 2 of manual quicksort (multithreaded)

    // pointers to hold important array information
    preSort* right = (preSort *) malloc(sizeof(preSort));
    preSort* left = (preSort *) malloc(sizeof(preSort));

    left->arr = t_buf1;
    left->low = low;
    left->high = pi-1;
    left->type = 1;
    
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

    free(left);
    free(right);

    return;
    /*
    // assign left vars
    left->arr = arr;
    left->nmemb = pivotPtr-arr;

    int diff = highestPtr - (pivotPtr + 1); 
    // assign right var
    right->arr = highestPtr - diff;
    right->nmemb = diff + 1;

    highestPtr = NULL;
    pivotPtr = NULL;
    */
}

short isDone(short ID) {
    return (sliceh.data[ID])->done;
}

short fillSlice(short ID) {
    slice *s = sliceh.data[ID];
    short fd = tsData.fd[ID];
    int bytes = 0;
    bytes = read(fd, s->arr, SLICE_BYTES);
    if (bytes == 0) {
        close(fd);
        //printf("File %d is finished!\n", fd);
        //fflush(stdout);
        s->size = -1;
        return -1;
    
    }
    //printf("fillslce called with ID: %d\n", ID);
    //fflush(stdout);
    s->size = bytes / 4;
    //printf("newBytes: %d\n", bytes);
    //fflush(stdout);
    s->readCount++;
    s->index = 0;
    return 1;
}

unsigned int getNumFromSlice(short ID) {
    slice *s = sliceh.data[ID];
    int tmp = s->index;
    unsigned int num = s->arr[tmp];
    s->index++;
    if (s->index == s->size) {
        short result = fillSlice(ID);
        if (result == -1) s->done = 1;
    }
    return num;
}

void initStage2(mheap *h) {
    // sliceh
    short fd = 0;
    int bytes = 0;
    // slices for days
    for (int i=0; i<tsData.fileIndex; i++) {
        // open file by name
        //printf("i: %d\n", i);
        //fflush(stdout);
        fd = open(tsData.name[i], O_RDONLY);
        
        // save its FD
        tsData.fd[i] = fd;

        // make slice and assign its vars
        slice *s = (slice *) malloc(sizeof(slice));
        s->ID = i;
        s->index = 0;
        s->done = 0;
        s->readCount = 1;
        sliceh.data[i] = s;
        bytes = read(fd, s->arr, SLICE_BYTES);
        s->size = bytes / 4;

        fflush(stdout);
        // make a node and add it to the min heap
        Node *node = (Node *) malloc(sizeof(Node));
        node->num = getNumFromSlice(s->ID);
        node->ID = i;
        insert(h, node);
        //printf("Slice size: %d; Slice index: %d\n", s->index, s->size);
    }
}

void writeOutput(int index, unsigned int *outBuf) {
    if (index < OUTSIZE)
        write(mainoutFD, outBuf, index*4);
    else 
        write(mainoutFD, outBuf, OUT_BYTES);
}

short setUpBogus() {
    // this function sets up a temporary file for intermidiary sorted data
    // along with data structures to track each one

    short index = tsData.fileIndex;
    char *tmp = (char *) malloc(sizeof(short));
    sprintf(tmp, "%d", index);
    short fd = open(tmp, O_WRONLY|O_CREAT, S_IRWXU);
    
    // size keeps track of how full each file is
    tsData.sizes[index] = prepArrSize;
    tsData.name[index] = tmp;
    return fd;
}

void setUpStuff() {
    prepArrSize = SZ; 
    for (int i=0; i<23; i++) {
        short fd = setUpBogus();
        close(fd);
        tsData.fileIndex++;
    }
}

int main(int argc, char** argv) {
    tsData.fileIndex = 0;
    if (argc > 1) {
        // open input
        maininFD = open(argv[1], O_RDONLY);
        //printf("my FD: %d\n", maininFD);
        //fflush(stdout);
    }

    else {
        printf("Error: no passed data files. Abort. \n");
        return 1;
    }
    /*
    unsigned int *tmparr = NULL;
    if (argc > 3) {
        stage1 = 0;
        setUpStuff();
    }
*/
    //tsData.fileIndex = 23;
    //mainBuf = buf1;
    //prepBuf = buf2;
    mainBuf = (unsigned int *) malloc(sizeof(unsigned int) * SZ);
    prepBuf = (unsigned int *) malloc(sizeof(unsigned int) * SZ);
    int mainDiff = 0;
    int Ldiff = 0;
    int Rdiff = 0;
    // read and fill the prep array, and swap it to primary array
    firstRead();
    swapArrays();

    // this is true until all pre-sorting is done
    while(stage1) {
        printf("Calling sort function...\n");
        fflush(stdout);
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
        
        t1->arr = t_buf1;
        t1->nmemb = LpivotPtr-t_buf1;


        t2->arr = t_buf2;
        t2->nmemb = Ldiff+1;
        
        t3->arr = t_buf3;
        t3->nmemb = mainDiff+1;
        
        t4->arr = t_buf4;
        t4->nmemb = Rdiff+1;

        printf("Launching threads...\n");
        fflush(stdout);
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
    printf("END OF STAGE 1. BEGIN STAGE 2. \n-------------------------------\n");
    fflush(stdout);
    // stage 2 
    stage2 = 1;
    free(mainBuf);
    free(prepBuf);
    mainoutFD = open(argv[2], O_WRONLY | O_APPEND | O_CREAT, S_IRWXU);
    unsigned int *outBuf = (unsigned int *) malloc(sizeof(unsigned int)* OUTSIZE);
    Node *node = NULL;
    int index = 0;
    short ID = 0;
    short done = 0;
    unsigned int tmp = 0;
    mheap* heap = CreateHeap();
    initStage2(heap);
    while(stage2) {
        while (index != OUTSIZE) {
            // get a node from the heap
            node = getMin(heap);
            if (node == NULL) {
                /*
                printf("ABOUT TO EXIT!!\n\n");
                fflush(stdout);
                printf("Info about heap: sz: %d", heap->count);
                for (int i=0; i<tsData.fileIndex; i++) {
                    slice* s = sliceh.data[i];
                    printf("EXIT INFO: size: %d; index: %d; ID: %d; readCount: %d\n", s->size, s->index, s->ID, s->readCount);
                    fflush(stdout);
                }
                */
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
                //verify(heap, node->num); 
            }
            else {
                // if it is, free it
                //printf("Be free, %d!\n", ID);
                //fflush(stdout);
                free(node);
                free(sliceh.data[ID]);
            }
        }
        printf("Writing to disk..,\n");
        fflush(stdout);
        writeOutput(index, outBuf);
        index = 0;
        if (done) stage2 = 0;
    }
    // free remaining variables
    for (int i=0; i<tsData.fileIndex; i++) {
        unlink(tsData.name[i]);
        free(tsData.name[i]);
    }
    free(outBuf);
    printf("Sorting complete. Exiting with status code 0.\n");
    return 0;
}
