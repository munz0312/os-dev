/* My implementation of paging
 * Initially I was using a 2 level page table (page directory -> page tables)
 * However there is an issue when trying to create new page table frames
 * When getting new pages from pmm_alloc_frame(), we get a physical addr,
 * but once we turn on paging in page_directory_init(), any physical addrs,
 * are treated as physical so...
 */

#include <kernel/paging.h>
#include <kernel/pmm.h>
#include <stdint.h>
#include <string.h>

uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096)));

void page_directory_init() {
    // bit 0 = present bit
    // bit 1 = r/w bit
    memset(page_directory, 0, sizeof(page_directory));
    page_directory[1023] = (uint32_t)page_directory | 0x3;
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

void map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    uint32_t pd_index = virtual_addr >> 22;
    uint32_t pt_index = (virtual_addr >> 12) & 0x03FF;

    uint32_t *pd = (uint32_t *)0xFFFFF000;
    uint32_t present = pd[pd_index] & 0x1;

    if (!present) {
        uint32_t new_table_frame = pmm_alloc_frame();
        pd[pd_index] = new_table_frame | flags;
        // Access through recursive mapping
        uint32_t *new_table = (uint32_t *)(0xFFC00000 + pd_index * 0x1000);
        memset(new_table, 0, 4096);
    }

    uint32_t *page_table_base = (uint32_t *)(0xFFC00000 + pd_index * 0x1000);
    page_table_base[pt_index] = physical_addr | flags;
    __asm__ volatile("invlpg (%0)" ::"r"(virtual_addr) : "memory");
}
