#ifndef MINHEAP_H
#define MINHEAP_H

#include <stdlib.h>
#include <stdio.h>
#include "structs.h"
#define MAX_CAPACITY 64

struct minHeap {
    Node* arr[MAX_CAPACITY];
    int count;
    int capacity;
} typedef mheap;

mheap *CreateHeap() {
    mheap *h = (mheap *) malloc(sizeof(mheap));
    h->count = 0;
    h->capacity = MAX_CAPACITY;
    return h;
}

void heapsort_bottom(mheap *h, int index) {
    Node *temp = NULL;
    int parent_node = (index - 1) / 2;

    if((h->arr[parent_node])->num > (h->arr[index])->num) {
        //swap and recursive call
        temp = h->arr[parent_node];
        h->arr[parent_node] = h->arr[index];
        h->arr[index] = temp;
        heapsort_bottom(h, parent_node);
    }
}

void insert(mheap *h, Node *node) {
    if( h->count < h->capacity){
        h->arr[h->count] = node;
        heapsort_bottom(h, h->count);
        h->count++;
        return;
    }
    printf("\nHEAP IS TOO FULL!!\n");
    fflush(stdout);
}

void heapsort_top(mheap *h, int parent_node) {
    int left = parent_node * 2 + 1;
    int right = parent_node * 2 + 2;
    int min;
    Node *temp;

    if (left >= h->count || left < 0)
        left = -1;
    
    if (right >= h->count || right < 0)
        right = -1;

    if (left != -1 && (h->arr[left])->num < (h->arr[parent_node])->num)
        min=left;

    else min = parent_node;

    if (right != -1 && (h->arr[right])->num < (h->arr[min])->num)
        min = right;

    if (min != parent_node){
        temp = h->arr[min];
        h->arr[min] = h->arr[parent_node];
        h->arr[parent_node] = temp;

        // recursive  call
        heapsort_top(h, min);
    }
}

Node* getMin(mheap *h) {
    Node *pop;
    if (h->count == 0){
        // heap is empty
        return NULL;
    }
    // replace first node by last and delete last
    pop = h->arr[0];
    h->arr[0] = h->arr[h->count-1];
    h->count--;
    heapsort_top(h, 0);
    return pop;
}

unsigned int peekTop(mheap *h) {
    return (h->arr[0])->num;
}

short verify(mheap *h, unsigned int num) {
    for (int i=0; i<h->count; i++) {
        if ((h->arr[i])->num == num) 
            return 1;
    }
    printf("VERY BAD! VERY BAD! ERROR!!!\n\n");
    fflush(stdout);
    return -1;
}

#endif 
