#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/keyboard.h>
#include <kernel/kmalloc.h>
#include <kernel/multiboot.h>
#include <kernel/paging.h>
#include <kernel/pic.h>
#include <kernel/pmm.h>
#include <kernel/serial.h>
#include <kernel/tty.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

void kernel_main(uint32_t magic, multiboot_info_t *mbd) {
    init_serial();
    populate_gdt();
    idt_init();
    PIC_remap(32, 40);

    pmm_init(mbd);
    page_directory_init();

    // divide error interrupt
    // __asm__ volatile("int $0");
    terminal_initialize();
    heap_init();
    init_keyboard();

    int *a = kmalloc(100);
    int *b = kmalloc(100);
    int *c = kmalloc(100);

    printf("a: %p\n", a);
    printf("b: %p\n", b);
    printf("c: %p\n", c);

    kfree(a);
    kfree(b);

    int *d = kmalloc(200);
    printf("d: %p\n", d);

    write_serial_string("Hello, host!\n");
    while (true) {
        __asm__ volatile("hlt");
    }
}
