#include "kernel/multiboot.h"
#include <kernel/pmm.h>
#include <stdint.h>
#include <string.h>

#define FREE 0x0
#define USED 0x1
#define NPAGES 32768

extern uint32_t endkernel;

static uint32_t bitmap[NPAGES / 32];

static void bitmap_set(uint32_t pfn) {
    // find which uint32_t in the bitmap the page frame is in.
    uint32_t index = pfn >> 5;

    // find page frame's corresponding bit within the 32-bit int.
    uint32_t bit = pfn & 31;

    bitmap[index] |= (USED << bit);
}

static void bitmap_clear(uint32_t pfn) {
    // find which uint32_t in the bitmap the page frame is in.
    uint32_t index = pfn >> 5;

    // find page frame's corresponding bit within the 32-bit int.
    uint32_t bit = pfn & 31;

    bitmap[index] &= ~(1 << bit);
}

static uint32_t bitmap_test(uint32_t pfn) {
    // find which uint32_t in the bitmap the page frame is in.
    uint32_t index = pfn >> 5;

    // find page frame's corresponding bit within the 32-bit int.
    uint32_t bit = pfn & 31;

    return bitmap[index] & (1 << bit);
}

void pmm_init(multiboot_info_t *mbd) {
    uint32_t endkernel_addr = (uint32_t)&endkernel;
    memset(bitmap, 0xFF, sizeof(bitmap));
    int i;
    for (i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t *mmmt =
            (multiboot_memory_map_t *)(mbd->mmap_addr + i);

        // printf("Start Addr: %lx | Length: %lx | Size: %x | Type: %d\n",
        //        mmmt->addr_low, mmmt->len_low, mmmt->size, mmmt->type);

        if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE) {
            uint32_t start_frame = mmmt->addr_low / 0x1000;
            uint32_t num_frames = mmmt->len_low / 0x1000;

            for (uint32_t f = 0; f < num_frames; f++) {
                bitmap_clear(start_frame + f);
            }
        }
    }

    uint32_t startkernel = 0x100000;
    uint32_t kpages = (endkernel_addr - startkernel + 0xFFF) / 0x1000;
    uint32_t startkernel_pf = startkernel / 0x1000;

    for (uint32_t f = 0; f < kpages; f++) {
        bitmap_set(startkernel_pf + f);
    }

    bitmap_set(0);
}

// Returns the (physical) base address to a newly allocated page frame
uint32_t pmm_alloc_frame() {
    uint32_t pfn = 0;

    while (bitmap_test(pfn)) {
        pfn++;
        if (pfn == NPAGES) {
            return -1;
        }
    }

    bitmap_set(pfn);

    return pfn * 0x1000;
}

void pmm_free_frame(uint32_t addr) {
    uint32_t pfn = addr / 0x1000;
    bitmap_clear(pfn);
}
