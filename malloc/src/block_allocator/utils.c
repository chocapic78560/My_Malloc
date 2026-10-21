#include "utils.h"

#include <stdio.h>

void read_data(struct blk_meta *meta)
{
    for (size_t i = 0; i < meta->size && meta->data[i] != '\0'; ++i)
        putchar(meta->data[i]);
}

void write_data(struct blk_meta *meta, char *data, size_t n)
{
    for (size_t i = 0; i < n; ++i)
        meta->data[i] = data[i];
    meta->data[n] = '\0';
}
