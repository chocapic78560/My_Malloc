#ifndef MALLOC_H
#define MALLOC_H

#include <stddef.h>

struct blk_meta
{
    size_t size;
    size_t free;
    struct blk_meta *next;
    struct blk_meta *prev;
    char data[];
};

struct page
{
    struct page *next;
    size_t size;
    struct blk_meta *first;
    size_t pad;
};

void *my_malloc(size_t size);
void my_free(void *ptr);
void *my_calloc(size_t nmemb, size_t size);
void *my_realloc(void *ptr, size_t size);

#endif /*Fin du header*/
