#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/keyboard.h>
#include <kernel/kmalloc.h>
#include <kernel/multiboot.h>
#include <kernel/paging.h>
#include <kernel/pic.h>
#include <kernel/pit.h>
#include <kernel/pmm.h>
#include <kernel/serial.h>
#include <kernel/thread.h>
#include <kernel/tty.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

void thread_b() {
    printf("hello from thread B!\n");
    while (true) {
        __asm__ volatile("hlt");
    }
}

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
    pit_setup();
    heap_init();
    init_keyboard();

    thread main_thread;
    main_thread.esp = 0;

    thread *b = thread_create("thread_b", thread_b);

    printf("switching to thread B...\n");
    switch_context(&main_thread.esp, &b->esp);
    printf("back in main!\n");

    write_serial_string("Hello, host!\n");
    while (true) {
        __asm__ volatile("hlt");
    }
}
