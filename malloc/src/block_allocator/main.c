#include <string.h>

#include "allocator.h"
#include "utils.h"

int main(void)
{
    struct blk_allocator *ba = blka_new();

    struct blk_meta *metas[6] = { NULL };
    char *messages[] = { "Hello world!\n",     "You are doing ", "a great job ",
                         "if you see ",        "this message ",  "correctly.\n",
                         "But do not forget ", "to release ",    "memory.\n" };

    // Allocate metadatas
    for (size_t i = 0; i < 3; ++i)
        metas[i] = blka_alloc(ba, 2048);
    for (size_t i = 3; i < 6; ++i)
        metas[i] = blka_alloc(ba, 4096);

    // Write and read messages to metadatas
    for (size_t i = 0; i < 6; ++i)
        write_data(metas[i], messages[i], strlen(messages[i]));
    for (size_t i = 0; i < 6; ++i)
        read_data(metas[i]);

    // Overwrite metadatas, pop and read them
    for (size_t i = 6; i < 9; ++i)
        write_data(metas[i - 6], messages[i], strlen(messages[i]));
    for (size_t i = 0; i < 3; ++i)
        blka_pop(ba);
    for (size_t i = 0; i < 3; ++i)
        read_data(metas[i]);

    // Release memory
    blka_delete(ba);

    return 0;
}
