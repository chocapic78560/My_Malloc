#include <stdio.h>

#include "page_begin.h"

static void display_result(void *ptr, size_t page_size, void *expected_result)
{
    void *res = page_begin(ptr, page_size);

    printf("ptr: %p\n", ptr);
    printf("page_size: %lu\n", page_size);
    printf("expected_result: %p\n", expected_result);
    printf("result: %p\n", res);

    printf(expected_result == res ? "OK\n" : "KO\n");
}

int main(void)
{
    display_result((void *)0x1234ffea, 4096, (void *)0x1234f000);
    display_result((void *)0x1234ffea, 256, (void *)0x1234ff00);
    return 0;
}
