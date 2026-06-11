#include <kernel/kmalloc.h>
#include <kernel/paging.h>
#include <kernel/pmm.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define START_ADDR 0x00400000

static uint32_t heap_boundary;

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

static int32_t grow_heap() {
    int32_t heap_page = pmm_alloc_frame();
    if (heap_page == -1)
        return -1;
    map_page(heap_boundary, heap_page, 0x3);
    memory_header *new = (memory_header *)heap_boundary;
    new->is_free = true;
    new->magic = 0xDEADBEEF;
    new->next = NULL;
    new->size = 4096 - sizeof(memory_header);

    memory_header *head = (memory_header *)START_ADDR;
    memory_header *last = head;
    for (; last->next != NULL; last = last->next)
        ;

    last->next = new;

    heap_boundary += 4096;
    return 0;
}

void coalesce() {
    memory_header *curr = (memory_header *)START_ADDR;
    while (curr != NULL) {
        while (curr->is_free && curr->next != NULL && curr->next->is_free) {
            curr->size += curr->next->size + sizeof(memory_header);
            curr->next = curr->next->next;
        }
        curr = curr->next;
    }
}

void *kmalloc(size_t requested_size) {

    while (true) {
        memory_header *head = (memory_header *)START_ADDR;

        for (memory_header *next = head; next != NULL; next = next->next) {

            if (!next->is_free || next->size < requested_size) {
                continue;
            }

            // if the block.size == requested_size then just return the pointer
            // to it
            if (next->size == requested_size) {
                next->is_free = false;
                return (void *)((char *)next + sizeof(memory_header));
            }

            // if the block is bigger then do a split to give the exact amount
            // needed (need at least requested_size + sizeof(memory_header))
            if (next->size >= (requested_size + sizeof(memory_header))) {
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

        // couldn't find a suitable memory block - try allocating more
        // memory to the heap
        if (grow_heap() == -1) {
            printf("no memory left");
            __asm__ volatile("cli; hlt");
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
        coalesce();
    } else {
        printf("invalid ptr: 0x%p\n", src);
        __asm__ volatile("cli; hlt");
    }
}
