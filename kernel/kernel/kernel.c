#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/keyboard.h>
#include <kernel/multiboot.h>
#include <kernel/paging.h>
#include <kernel/pic.h>
#include <kernel/pmm.h>
#include <kernel/serial.h>
#include <kernel/tty.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
    init_keyboard();

    write_serial_string("Hello, host!\n");
    while (true) {
        __asm__ volatile("hlt");
    }
}
