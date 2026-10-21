#include "alignment.h"

#include <stddef.h>

size_t align(size_t size)
{
    size_t res;

    if (__builtin_add_overflow(size, sizeof(long double) - 1,
                               &res)) // check overflow
        return 0;
    return res / sizeof(long double)
        * sizeof(long double); // division entiere puis multiple le plus grand
}
