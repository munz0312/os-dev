#include <kernel/idt.h>
#include <kernel/keyboard.h>
#include <kernel/pic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct __attribute__((packed)) {
    uint16_t offset_low;
    uint16_t kernel_cs;
    uint8_t reserved;
    uint8_t attributes;
    uint16_t offset_high;
} IDT_entry;

typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint32_t base;
} IDTR;

__attribute__((aligned(0x10))) static IDT_entry idt[256];

void isr_dispatch(uint32_t vector, uint32_t error_code) {

    switch (vector) {

    // page fault
    case 14: {
        uint32_t faulting_addr;
        // virtual address that caused the fault is in CR2
        __asm__ volatile("mov %%cr2, %0" : "=r"(faulting_addr));
        printf("page fault caused by address: 0x%x", faulting_addr);

        // disable interrupts and halt execution (for now)
        __asm__ volatile("cli; hlt");
        break;
    }

    case 33: {
        keyboard_handler();
        break;
    }

    default:
        break;
    }
    if (vector >= 32)
        PIC_sendEOI(vector - 32);
    return;
}

void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags);

void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags) {
    IDT_entry *descriptor = &idt[vector];
    descriptor->offset_low = (uint32_t)isr & 0xFFFF;
    descriptor->kernel_cs = 0x08;
    descriptor->attributes = flags;
    descriptor->offset_high = (uint32_t)isr >> 16;
    descriptor->reserved = 0;
}

extern void *isr_stub_table[];
static bool vectors[IDT_MAX_DESCRIPTORS];
static IDTR idtr;

void idt_init() {
    idtr.base = (uintptr_t)&idt[0];
    idtr.limit = (uint16_t)sizeof(IDT_entry) * IDT_MAX_DESCRIPTORS - 1;

    for (uint8_t vector = 0; vector < 48; vector++) {
        idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
        vectors[vector] = true;
    }

    __asm__ volatile("lidt %0" : : "m"(idtr)); // load the new IDT
    __asm__ volatile("sti");                   // set the interrupt flag
}
