#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/pic.h>
#include <kernel/serial.h>
#include <kernel/tty.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

void kernel_main(void) {
    init_serial();
    populate_gdt();
    idt_init();
    PIC_remap(32, 40);
    // divide error interrupt
    // __asm__ volatile("int $0");
    terminal_initialize();
    printf("Hello, kernel World!\n");
    write_serial_string("Hello, host!\n");
    while (true) {
        __asm__ volatile("hlt");
    }
}
