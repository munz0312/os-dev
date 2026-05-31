#include <kernel/paging.h>
#include <stdint.h>
uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096)));

void page_directory_init() {
    page_directory[0] = ((uint32_t)first_page_table) | 0x3;
    for (int i = 0; i < 1024; i++) {
        first_page_table[i] = (i * 0x1000) | 0x3;
    }

    // Set page directory base register
    __asm__ volatile("mov %0, %%cr3" ::"r"(page_directory));
    // Load cr0
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    // Set bit 31 (PG paging bit) and writeback
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0" ::"r"(cr0));
}
