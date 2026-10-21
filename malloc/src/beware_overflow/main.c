#include <stddef.h>
#include <stdio.h>

#include "beware_overflow.h"

void print_overflow(void *ptr, size_t nmemb, size_t size)
{
    void *begin = ptr;
    void *res = beware_overflow(ptr, nmemb, size);
    if (res)
        printf("Pointer was incremented from %p to %p.\n", begin, res);
    else
        printf("Overflow detected between %ld and %ld.\n", nmemb, size);
}

int main(void)
{
    print_overflow((void *)0x1000, 25, 6);
    print_overflow((void *)0x40000, 12345678904, 12345678904);
    return 0;
}
