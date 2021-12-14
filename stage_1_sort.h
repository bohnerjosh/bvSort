#ifndef STAGE_1_SORT_H
#define STAGE_1_SORT_H

// external global variables
extern unsigned int *pivotPtr;
extern unsigned int *highestPtr;
extern unsigned int *LpivotPtr;
extern unsigned int *LhighestPtr;
extern unsigned int *RpivotPtr;
extern unsigned int *RhighestPtr;
#include "structs.h"

// comparator for qsort
static int cmpInt(const void *p1, const void *p2) {
    // cast to int pointer plus dereference
    unsigned int aa = *(unsigned int *)p1;
    unsigned int bb = *(unsigned int *)p2;
   
    // compare 
    if (aa > bb) return 1;
    else if (aa < bb) return -1;
    else return 0; // equal
}
// swaps elements in the main array at the given pointers
void swap(unsigned int *a, unsigned int *b) {
    unsigned int t = *a;
    *a = *b;
    *b = t;
}

int partition_t(unsigned int *arr, int low, int high, short type) {

    // assign pivot to last element and move it
    unsigned int pivot = arr[high];
    unsigned int t = 0;
    int i = (low - 1);
    
    // partition each element such that all elements less than the pivot are moved to the left
    // if the number is greater, it stays
    for (int j=low; j<high; j++) {
        if (arr[j] < pivot) {
            i++;
            // manually swap i and j for performance
            t = arr[i];
            arr[i] = arr[j];
            arr[j] = t;
            t = 0;
        }
    }
    // swap the pivot element with the greater element at i
    swap(&arr[i+1], &arr[high]);
    // new pivot and pointers

    // depending on the array that called this func, assign pointers
    if (type == 1) {
        LpivotPtr = &(arr[i+1]);
        LhighestPtr = &(arr[high]);
    }
    else {
    
        RpivotPtr = &(arr[i+1]);
        RhighestPtr = &(arr[high]);
    }
    // return new pivot
    return (i+1);
}

int partition(unsigned int *arr, int low, int high) {
    
    // assign pivot to last element and move it
    unsigned int pivot = arr[high];
    unsigned int t = 0;
    int i = (low - 1);
    
    // partition each element such that all elements less than the pivot are moved to the left
    // if the number is greater, it stays
    for (int j=low; j<high; j++) {
        if (arr[j] < pivot) {
            i++;

            // manually swap i and j for performance
            t = arr[i];
            arr[i] = arr[j];
            arr[j] = t;
            t = 0;
        }
    }
    // swap the pivot element with the greater element at i
    swap(&arr[i+1], &arr[high]);
    // new pivot and pointers
    pivotPtr = &(arr[i+1]);
    highestPtr = &(arr[high]);
    return (i+1);
}

#endif
