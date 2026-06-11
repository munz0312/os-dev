#include <stddef.h>

void heap_init();
void *kmalloc(size_t requested_size);
void kfree(void *src);
