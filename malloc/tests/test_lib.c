#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static int check(const void *p, size_t n,
                 unsigned char v) // check si chaque octet a pour valeur v
{
    const unsigned char *c = p;
    for (size_t i = 0; i < n; i++)
    {
        if (c[i] != v)
            return 0;
    }
    return 1;
}

static void test_malloc(void)
{
    for (size_t s = 1; s <= 300; s++)
    {
        unsigned char *p = malloc(s); // cree chaque pointeur
        assert(p);
        assert((size_t)p % 16
               == 0); // pour tester que c bien aligne sur sizeof(long double)
        memset(p, 0xAB, s);
        assert(check(p, s, 0xAB));
        free(p);
    }
}

static void test_two_blocks(void)
{
    unsigned char *a = malloc(15);
    unsigned char *b = malloc(25);
    assert(a && b && a != b);
    memset(a, 1, 15);
    memset(b, 2, 25);
    assert(check(a, 15, 1) && check(b, 25, 2));
    free(a);
    free(b);
}

static void test_calloc(void)
{
    void *s = malloc(16);
    unsigned char *p = malloc(100);
    memset(p, 0xFF, 100);
    free(p);
    unsigned char *q = calloc(25, 4);
    assert(q && check(q, 100, 0));
    free(q);
    free(s);
    volatile size_t big = (size_t)-1;
    assert(calloc(big, 2) == NULL);
}

static void test_realloc(void)
{
    unsigned char *p = malloc(100);
    memset(p, 0xAB, 100);
    unsigned char *q = realloc(p, 5000);
    assert(q
           && check(q, 100,
                    0xAB)); // pour check que ça garde bien l ancienne memoire
    q = realloc(q, 50);
    assert(q && check(q, 50, 0xAB)); // verifie tjrs que ça garde la memoire
    free(q);
    void *final = realloc(NULL, 32);
    assert(final != NULL); // cense malloc selon la manpages
    free(final);
}

static void test_big(void)
{
    size_t n = (size_t)1 << 20; // 2 a la puissance 20
    unsigned char *p = malloc(n);
    assert(p);
    memset(p, 0x33, n);
    assert(check(p, n, 0x33)); // verfie que c bien rempli
    free(p);
}

static void malloc_test_edge_case(void)
{
    assert(malloc(0) == NULL);
    void *p = malloc(128);
    assert(p != NULL);
    free(p);
}

static void retest_realloc(void)
{
    void *p = malloc(25);
    p = realloc(p, 0);
    assert(p == NULL);
    void *malloced = realloc(NULL, 128);
    assert(malloced != NULL); // verfie que ça malloc bien
    free(malloced);
}

static void calloc_edge_cases(void)
{
    void *test = calloc(0, 0);
    assert(test == NULL);
    volatile size_t max =
        __SIZE_MAX__; // floctiboxbox m a dit volatile pour compiler
    test = calloc(max, max);
    assert(test == NULL); // check overflow
    test = calloc(2, 2);
    assert(test != NULL); // calloc marche bien
    free(test);
}

static void test_overflow_and_free_null(void)
{
    volatile size_t big = (size_t)-1;
    assert(malloc(big) == NULL);
    free(NULL);
}

static void test_merge_reuse(void)
{
    void *s = malloc(16);
    void *a = malloc(64);
    void *b = malloc(64);
    void *c = malloc(64);
    void *g = malloc(16);
    free(a);
    free(c);
    free(b);
    void *big = malloc(256);
    assert(big == a);
    free(big);
    free(g);
    free(s);
}

static void bon_fonctionnement_free(void)
{
    void *s = malloc(16);
    void *first = malloc(100);
    assert(first != NULL);
    size_t addr = (size_t)first;
    void *new = realloc(first, 0); // marque first comme free
    assert(new == NULL);
    void *second = malloc(100);
    assert(second != NULL);
    assert((size_t)second == addr); // est cense etre au même endroit que le
                                    // premier car il a été marque comme free
    free(second);
    free(s);
}
int main(void)
{
    test_malloc();
    test_two_blocks();
    test_calloc();
    test_realloc();
    test_big();
    malloc_test_edge_case();
    retest_realloc();
    calloc_edge_cases();
    test_overflow_and_free_null();
    test_merge_reuse();
    bon_fonctionnement_free();
    return 0;
}
