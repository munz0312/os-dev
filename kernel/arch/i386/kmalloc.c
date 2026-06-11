#include <kernel/kmalloc.h>
#include <kernel/paging.h>
#include <kernel/pmm.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define START_ADDR 0x00400000

uint32_t heap_boundary;

typedef struct memory_header {
    size_t size;
    struct memory_header *next;
    uint32_t magic;
    bool is_free;
} memory_header;

void heap_init() {
    uint32_t heap_page = pmm_alloc_frame();
    map_page(START_ADDR, heap_page, 0x3);
    memory_header *head = (memory_header *)START_ADDR;
    head->size = 4096 - sizeof(memory_header);
    head->next = NULL;
    head->magic = 0xDEADBEEF;
    head->is_free = true;
    heap_boundary = START_ADDR + 4096;
}

void *kmalloc(size_t requested_size) {
    memory_header *head = (memory_header *)START_ADDR;

    for (memory_header *next = head; next != NULL; next = next->next) {

        if (!next->is_free) {
            continue;
        }

        // if the block.size == requested_size then just return the pointer to
        // it
        if (next->size == requested_size) {
            next->is_free = false;
            return (void *)((char *)next + sizeof(memory_header));
        }

        // if the block is bigger then do a split to give the exact amount
        // needed (need at least requested_size + sizeof(memory_header))
        if (next->size > (requested_size + sizeof(memory_header))) {
            memory_header *new =
                (memory_header *)((char *)next + sizeof(memory_header) +
                                  requested_size);
            new->size = next->size - requested_size - sizeof(memory_header);
            new->is_free = true;
            new->magic = 0xDEADBEEF;
            new->next = next->next;

            next->size = requested_size;
            next->next = new;
            next->is_free = false;
            return (void *)((char *)next + sizeof(memory_header));
        }
    }

    // unsuccessful
    return NULL;
}

void kfree(void *src) {
    memory_header *header =
        (memory_header *)((char *)src - sizeof(memory_header));
    if ((uint32_t)header < heap_boundary && (uint32_t)header >= START_ADDR &&
        header->magic == 0xDEADBEEF) {
        header->is_free = true;
    } else {
        printf("invalid ptr: 0x%p\n", src);
        __asm__ volatile("cli; hlt");
    }
}
