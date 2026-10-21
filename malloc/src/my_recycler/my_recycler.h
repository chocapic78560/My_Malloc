#ifndef MY_RECYCLER_H
#define MY_RECYCLER_H

#include <stddef.h>

struct recycler
{
    size_t block_size; // size of a block
    size_t capacity; // number of blocks in the chunk
    void *chunk; // memory chunk containing all blocks
    void *free; // address of the first free block of the free list
};

struct free_list
{
    struct free_list *next; // next free block
};

struct recycler *recycler_create(size_t block_size, size_t total_size);
void recycler_destroy(struct recycler *r);
void *recycler_allocate(struct recycler *r);
void recycler_free(struct recycler *r, void *block);

#endif /* ! MY_RECYCLER_H */
