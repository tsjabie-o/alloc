#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define TRUE 1
#define FALSE 0

// Doubly linked list
struct node {
    size_t size;
    struct node *next;
    struct node *prev;
    int is_free;
};
struct node *start;
struct node *end;

struct node *askmem(size_t size, struct node *curr) {
    size_t paddedsize_node = sizeof(struct node) + (8-(sizeof(struct node)%8));
    struct node *_node = (char*)curr + paddedsize_node + curr->size;

    if (!(curr != NULL && (char*)sbrk(0) - (char*)curr + paddedsize_node + curr->size >= paddedsize_node + size)) {
        size_t actsize = 2048;
        while (size >= actsize) {
            actsize *= 2;
        }
        sbrk(actsize);
    }
    // struct node *_rest = (struct node*)((char*)_node + size + paddedsize_node);

    _node->is_free = 0;
    _node->prev = curr;
    _node->next = NULL;
    _node->size = size;

    // _rest->is_free = 1;
    // _rest->next = NULL;
    // _rest->prev = _node;
    // _rest->size = actsize - size - 2*paddedsize_node;
    // end = _rest;

    return _node;
}

void *mymalloc(size_t size)
{
    void *p;

    // Extending both node and data to multiple of 8 bytes so always aligned
    size_t paddedsize_data = size + (8-(size%8));
    size_t paddedsize_node = sizeof(struct node) + (8-(sizeof(struct node)%8));


    if (!start) {
        // Create first block and return pointer
        start = askmem(paddedsize_data, NULL);
        p = (char*)start + paddedsize_node;

        return p;
    } else {
        // Find a free block
        struct node *curr = start;
        while (TRUE)
        {
            if (curr->is_free == 1 && curr->size >= paddedsize_data) {
                // Found free block

                curr->is_free = 0;
                p = (char*)curr + paddedsize_node;

                // ? Split free segment
                if (curr->size > paddedsize_data + paddedsize_node + 64) {
                    // If the free chunk is large enough (>data+node) split it
                    
                    size_t leftover = curr->size - (paddedsize_data + paddedsize_node);

                    struct node *newnode = (struct node*)((char*)curr + paddedsize_data + paddedsize_node);
                    curr->size = paddedsize_data;
                    newnode->size = leftover;
                    newnode->is_free = 1;
                    newnode->prev = curr;
                    newnode->next = curr->next;
                    if (newnode->next != NULL) {newnode->next->prev = newnode;} else {end = newnode;}
                    curr->next = newnode;
                }
                return p;
            }
            if (!curr->next) {
                curr->next = askmem(paddedsize_data, curr);
                p = (char*)curr->next + paddedsize_node;
                return p;
            }
            // Try next block
            curr = curr->next;
        }
    }
}

void *mycalloc(size_t nmemb, size_t size)
{
    if((size_t)-1 / nmemb < size) { return NULL;}
    void *p = mymalloc(nmemb * size);
    memset(p, 0, size*nmemb);
    
    return p;
}

void myfree(void *ptr)
{
    // Find the node of this data
    size_t paddedsize_node = sizeof(struct node) + (8-(sizeof(struct node)%8));
    struct node *_node = (struct node*)((char*)ptr - paddedsize_node);
    
    // give back mem if last
    // if (_node->next == NULL && _node->size >= 256) {
    //     printf("neuk");
    //     sbrk(-1*(paddedsize_node + _node->size));
    //     return;
    // }

    _node->is_free = 1;
    // ? Check for merge
    if (_node->next != NULL) {
        if (_node->next->is_free == 1){
            _node->size = _node->size + _node->next->size + paddedsize_node;
            _node->next = _node->next->next;
            if(_node->next != NULL) {
                _node->next->prev = _node;
            } else {
                end = _node;
            }
        }
    }
    if (_node->prev != NULL) {
        if (_node->prev->is_free == 1){
            _node->prev->size = _node->prev->size + _node->size + paddedsize_node;
            _node->prev->next = _node->next;
            if (_node->next != NULL) {
                _node->next->prev = _node->prev;
            } else {
                end = _node;
            }
            _node = _node->prev;
        }
    }
    return;
}

void *myrealloc(void *ptr, size_t size)
{
    size_t paddedsize_node = sizeof(struct node) + (8-(sizeof(struct node)%8));
    size_t paddedsize_data = size + (8-(size%8));

    struct node *_node = (struct node*)((char*) ptr - paddedsize_node);


    if (ptr == NULL){
        return mymalloc(size);
    } else if (size == 0) {
        myfree(ptr);
        return NULL;
    } else {
        if (paddedsize_data <= _node->size) {
            // Already fit
            return ptr;
        }
        // check if free chunk next to this chunk
        if (_node->next == NULL){
            // we can simply extend brk
            size_t diff = paddedsize_data - _node->size;
            sbrk(diff);
            _node->size = _node->size + diff;
            return ptr;
        }
        if (_node->next->is_free == 1 &&
            _node->next->size >= paddedsize_data - _node->size - paddedsize_node){
            // remove next node and give back original ptr
            if (_node->next->next != NULL) {_node->next->next->prev = _node;}
            _node->size = _node->size + _node->next->size + paddedsize_node;
            _node->next = _node->next->next;
            return ptr;
        } else {
            void *p = mymalloc(paddedsize_data);
            memcpy(p, ptr, paddedsize_data);
            myfree(ptr);
            return p;
        }
    }
}


/*
 * Enable the code below to enable system allocator support for your allocator.
 * Doing so will make debugging much harder (e.g., using printf may result in
 * infinite loops).
 */
#if 0
void *malloc(size_t size) { return mymalloc(size); }
void *calloc(size_t nmemb, size_t size) { return mycalloc(nmemb, size); }
void *realloc(void *ptr, size_t size) { return myrealloc(ptr, size); }
void free(void *ptr) { myfree(ptr); }
#endif
