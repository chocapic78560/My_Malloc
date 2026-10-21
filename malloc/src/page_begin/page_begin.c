#include <stddef.h>

void *page_begin(void *ptr, size_t page_size)
{
    char *p = ptr; // cast implicite pour avoir octet par octet
    size_t offset = (size_t)p
        & (page_size - 1); // calcul du offset calcul trouve sur internet
    return p - offset;
}
