#include <stdio.h>

#include "alignment.h"

static void display_result(size_t size, size_t expected_result)
{
    size_t res = align(size);

    printf("initial size: %lu\n", size);
    printf("expected_result: %lu\n", expected_result);
    printf("result: %lu\n", res);

    printf(expected_result == res ? "OK\n" : "KO\n");
}

int main(void)
{
    display_result(5, sizeof(long double));

    return 0;
}
