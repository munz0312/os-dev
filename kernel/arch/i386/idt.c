#include "kernel/tty.h"
#include "sys/io.h"
#include <kernel/idt.h>
#include <kernel/pic.h>
#include <stdbool.h>
#include <stdint.h>

char scan_code_to_ascii[128] = {
    0,   0,   '1',  '2',  '3',  '4', '5', '6',  '7', '8', '9', '0',
    '-', '=', '\b', '\t', 'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
    'o', 'p', '[',  ']',  '\n', 0,   'a', 's',  'd', 'f', 'g', 'h',
    'j', 'k', 'l',  ';',  '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm',  ',',  '.',  '/', 0,   '*',  0,   ' ',
};

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

void isr_dispatch(uint32_t vector) {
    if (vector < 32) {
        __asm__ volatile("cli; hlt");
    } else {
        uint8_t scan_code;
        char c;
        switch (vector) {

        case 33:
            scan_code = inb(0x60);
            if (scan_code & 0x80)
                break;
            c = scan_code_to_ascii[scan_code];
            if (c != 0)
                terminal_putchar(c);
            break;

        default:
            break;
        }
        PIC_sendEOI(vector - 32);
        return;
    }
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
