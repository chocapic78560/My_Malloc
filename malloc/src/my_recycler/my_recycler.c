#include "my_recycler.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

struct recycler *recycler_create(size_t block_size, size_t total_size)
{
    if (!block_size || !total_size)
        return NULL;
    if (block_size % sizeof(long double) != 0 || total_size % block_size != 0)
        return NULL;
    struct recycler *new = malloc(sizeof(struct recycler));
    if (!new)
        return NULL;
    new->block_size = block_size;
    new->capacity = total_size / block_size;
    void *chunks = malloc(total_size);
    if (!chunks)
    {
        free(new); // pour eviter les memories leaks
        return NULL;
    }
    new->chunk = chunks;
    new->free = chunks;
    struct free_list *cur = new->chunk;
    for (size_t i = 1; i < new->capacity; i++)
    {
        void *tmp = cur;
        char *casted = tmp;
        void *decalage = casted + block_size;
        cur->next = decalage; // on avance de block_size a chaque fois
        cur = cur->next;
    }
    cur->next = NULL; // pour que le dernier pointe vers NULL
    new->free = new->chunk; // pointe vers le premier
    return new;
}

void recycler_destroy(struct recycler *r)
{
    if (!r)
        return;
    free(r->chunk);
    free(r);
}

void *recycler_allocate(struct recycler *r)
{
    if (!r || !r->free)
        return NULL;
    if (!r->capacity)
        return NULL;
    struct free_list *first_block = r->free;
    r->free = first_block->next;
    return first_block;
}

void recycler_free(struct recycler *r, void *block)
{
    if (!r || !block)
        return;
    struct free_list *old_head = r->free;
    struct free_list *new_head = block;
    new_head->next = old_head;
    r->free = new_head;
}
