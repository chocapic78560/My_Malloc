#include "allocator.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "utils.h"
struct blk_allocator *blka_new(void)
{
    struct blk_allocator *new = malloc(sizeof(struct blk_allocator));
    if (!new)
        return NULL;
    new->meta = NULL;
    return new;
}
struct blk_meta *blka_alloc(struct blk_allocator *blka, size_t size)
{
    size_t page_size = sysconf(_SC_PAGESIZE);
    size_t total = size + sizeof(struct blk_meta); // la page + les metadonnees

    size_t nb_pages = (total + page_size - 1) / page_size;
    size_t map_size = nb_pages * page_size;

    void *addr = mmap(NULL, map_size, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0); // mmap des slides
    if (addr == MAP_FAILED) // erreur
        return NULL;

    struct blk_meta *blk = addr; // implicite
    blk->size = map_size - sizeof(struct blk_meta); // sans les metadonnees
    blk->next = blka->meta;
    blka->meta = blk;
    return blk;
}

void blka_free(struct blk_meta *blk)
{
    munmap(blk, blk->size + sizeof(struct blk_meta));
}
void blka_pop(struct blk_allocator *blka)
{
    if (!blka || !blka->meta)
        return;
    struct blk_meta *first = blka->meta;
    blka->meta = blka->meta->next;
    blka_free(first);
}
void blka_delete(struct blk_allocator *blka)
{
    while (blka->meta)
    {
        blka_pop(blka);
    }
    free(blka);
}
