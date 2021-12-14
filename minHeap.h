#ifndef MINHEAP_H
#define MINHEAP_H

#include <stdlib.h>
#include <stdio.h>
#include "structs.h"
#define MAX_CAPACITY 64

// hey, its a min heap!
// stores Nodes, which contain an unsigned integer and an ID
// representing the temp file it came from
struct minHeap {
    Node* arr[MAX_CAPACITY];
    int count; // number of elements
    int capacity; // capacity of heap
} typedef mheap;

mheap *CreateHeap() {
    // creates a heap
    mheap *h = (mheap *) malloc(sizeof(mheap));
    h->count = 0;
    h->capacity = MAX_CAPACITY;
    return h;
}

void heapsort_bottom(mheap *h, int index) {
    // sorts the heap from the bottom up
    Node *temp = NULL;
    int parent_node = (index - 1) / 2;

    // if the parent node's num is larger than the index of the node passed in
    if((h->arr[parent_node])->num > (h->arr[index])->num) {
        //swap and recursive call
        temp = h->arr[parent_node];
        h->arr[parent_node] = h->arr[index];
        h->arr[index] = temp;
        heapsort_bottom(h, parent_node);
    }
}

void insert(mheap *h, Node *node) {
    // inserts a node into the heap
    if( h->count < h->capacity){
        h->arr[h->count] = node;
        heapsort_bottom(h, h->count);
        h->count++;
        return;
    }
}

void heapsort_top(mheap *h, int parent_node) {
    // sorts the heap from the top down

    // find num of layers on each side
    int left = parent_node * 2 + 1;
    int right = parent_node * 2 + 2;
    int min; // minimum element
    Node *temp = NULL;

    // check for balance, -1 means balanced
    if (left >= h->count || left < 0)
        left = -1;
    
    if (right >= h->count || right < 0)
        right = -1;

    // if not balanced, min is set
    if (left != -1 && (h->arr[left])->num < (h->arr[parent_node])->num)
        min=left;

    else min = parent_node;

    // if not balanced, min is set
    if (right != -1 && (h->arr[right])->num < (h->arr[min])->num)
        min = right;

    // recursive check for balance and sort
    if (min != parent_node){
        temp = h->arr[min];
        h->arr[min] = h->arr[parent_node];
        h->arr[parent_node] = temp;

        heapsort_top(h, min);
    }
}

Node* getMin(mheap *h) {
    // gets the min element
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

#endif 
