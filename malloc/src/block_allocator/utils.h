#ifndef UTILS_H
#define UTILS_H

#include "allocator.h"

/*
 * Print datas the `meta` block contains.
 */
void read_data(struct blk_meta *meta);

/*
 * Write `data` in the `meta` block.
 */
void write_data(struct blk_meta *meta, char *data, size_t n);

#endif /* !UTILS_H  */
