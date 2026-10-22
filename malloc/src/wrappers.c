#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "alignment/alignment.h"
#include "malloc.h"

static struct page *pages = NULL;

static struct blk_meta *meta_of(void *ptr) // retire les metadata pour être au debut du bloc
{
    char *c = ptr;
    c -= sizeof(struct blk_meta);
    void *v = c;
    return v;
}

static struct page *page_of(struct blk_meta *b) // retire l entete vrmt tout au debut de la page
{
    while (b->prev)
        b = b->prev;
    void *tmp = b;
    char *c = tmp;
    c -= sizeof(struct page);
    void *v = c;
    return v;
}

// sert a ajouter une nouvelle page de taille size pour le malloc ou cas ou le block n existe pas
static struct page *page_new(size_t size)
{
    size_t total;
    if (__builtin_add_overflow(
            size, sizeof(struct page) + sizeof(struct blk_meta), &total))
        return NULL;
    long test = sysconf(_SC_PAGE_SIZE); // taille de la page
    if (test == -1)
        return NULL;
    size_t page_size = test;
    total = (total + page_size - 1) / page_size * page_size; // calcul de la taille qu il faut mapper
    void *addr = mmap(NULL, total, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (addr == MAP_FAILED)
        return NULL;
    char *casted = addr; // implicite
    struct page *page = addr;
    void *beginning = casted + sizeof(struct page); // debut du blocks
    struct blk_meta *new = beginning;
    new->free = 1;
    new->next = NULL;
    new->prev = NULL;
    new->size = total - sizeof(struct page) - sizeof(struct blk_meta); // taille totale des block sans en-tete et metadata
    page->size = total; // taille de la page mappee
    page->pad = 0;
    page->first = new;
    page->next = pages;
    pages = page;
    return page;
}
// test si le bloc est dans une des  page pour savoir si il a été alloué par nous
static struct page *block_in(struct blk_meta *b)
{
    void *tmp = b;
    char *c = tmp;
    for (struct page *p = pages; p; p = p->next)
    {
        void *casted = p;
        char *debut = casted;
        char *fin = debut + p->size;
        if (c >= debut && c < fin)
            return p;
    }
    return NULL;
}

static struct blk_meta *find_fit(size_t size) // trouve le premier bloc avec une taille >=size
{
    for (struct page *p = pages; p; p = p->next)
    {
        for (struct blk_meta *b = p->first; b; b = b->next)
        {
            if (b->free && b->size >= size)
                return b;
        }
    }
    return NULL;
}

static void page_free(struct page *p) // libere une page et l enleve de la liste chainee
{
    struct page **list = &pages;
    while (*list && *list != p)
        list = &(*list)->next;
    *list = p->next;
    munmap(p, p->size);
}

// garde uniquement les octets necessaires et donne le reste a son next et link le next
static void block_split(struct blk_meta *b, size_t size)
{
    size_t reste = b->size - size;
    size_t meta_entete = sizeof(struct blk_meta)+ sizeof(long double); // sans les metadata et les 16 minimum
    if (reste < meta_entete)
        return;
    void *tmp = b;
    char *casted = tmp;
    void *next = casted + sizeof(struct blk_meta) + size;
    struct blk_meta *nv = next;
    nv->free = 1;
    nv->prev = b;
    nv->next = b->next;
    nv->size = reste - sizeof(struct blk_meta); // reste - les metadatas
    if (nv->next)
        nv->next->prev = nv;
    b->size = size;
    b->next = nv;
}

static void block_merge(struct blk_meta *b) // fusionne b avec son prochain
{
    struct blk_meta *n = b->next;
    if (!n)
        return;
    b->size += sizeof(struct blk_meta) + n->size;
    b->next = n->next;
    if (n->next)
        n->next->prev = b;
}

static int block_can_extend(struct blk_meta *b,size_t size) // si le bloc + son prochain >= size
{
    return b->next && b->next->free && (b->size + b->next->size + sizeof(struct blk_meta)) >= size;
}

void *my_malloc(size_t size)
{
    size_t allignee = align(size);
    if (!allignee)
        return NULL;
    struct blk_meta *block = find_fit(allignee);
    if (block)
    {
        block_split(block, allignee); // mecanique de split pour optimiser et prendre que les octets necessaires
        block->free = 0;
        return block->data;
    }
    struct page *page = page_new(size); // mmap uniquement si y a pas le bloc
    if (!page)
        return NULL;
    block = page->first;
    block_split(block, allignee); // mecanique de split pour optimiser et
                                  // prendre que les octets necessaires
    block->free = 0;
    return block->data;
}

void my_free(void *ptr)
{
    if (!ptr)
        return;
    struct blk_meta *beginning =meta_of(ptr); // recule des metadonnees pour arriver au tout debut de la structure
    if (!block_in(beginning))
        return;
    beginning->free = 1;
    if (beginning->next && beginning->next->free)
    {
        block_merge(beginning);
    }
    if (beginning->prev && beginning->prev->free)
    {
        beginning = beginning->prev;
        block_merge(beginning);
    }
    if (!beginning->prev && !beginning->next)
        page_free(page_of(beginning)); // libere l entierete de la page
}

void *my_calloc(size_t nmemb, size_t size)
{
    size_t total;
    if (__builtin_mul_overflow(nmemb, size, &total))
        return NULL;
    void *ptr = my_malloc(total);
    if (!ptr)
        return NULL;
    memset(ptr, 0, total);
    return ptr;
}

void *my_realloc(void *ptr, size_t size)
{
    if (!ptr) // manpage
        return my_malloc(size);
    if (!size) // manpage
    {
        my_free(ptr);
        return NULL;
    }
    size = align(size);
    if (!size) // overflow
        return NULL;
    struct blk_meta *p = meta_of(ptr);
    if (!block_in(p)) // si c un pointeur pas alloue par nous
        return NULL;
    if (p->size >= size) // il faut split la taille est deja trop grande
    {
        block_split(p, size);
        return ptr;
    }
    if (block_can_extend(p, size)) // au cas ou y a pas assez de taille pour eviter de malloc
    {
        block_merge(p); // on merge avec le prochain
        block_split(p, size); // puis split pour l opti prcq on est des bg
        return ptr;
    }
    void *new = my_malloc(size); // sinon faut malloc
    if (!new) // erreur de malloc
        return NULL;
    memcpy(new, ptr, p->size); // copie de la memoire de l ancien bloc
    my_free(ptr);
    return new;
}
