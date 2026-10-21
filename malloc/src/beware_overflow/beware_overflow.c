#include "beware_overflow.h"

#include <stddef.h>
#include <stdio.h>
#include <sys/mman.h>

void *beware_overflow(void *ptr, size_t nmemb, size_t size)
{
    size_t total;
    if (__builtin_mul_overflow(
            nmemb, size,
            &total)) // c un builin gcc qui check l overflow pour nous mrc
        return NULL;
    char *final = ptr;
    return final + total;
}
