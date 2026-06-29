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

void thread_sleeper() {
    unlock_scheduler();
    for (int i = 0; i < 3; i++) {
        printf("sleeper %d: going to sleep at tick %llu\n", i,
               get_current_ticks());
        nanosleep(50);
        printf("sleeper %d: woke up at tick %llu\n", i, get_current_ticks());
    }

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

    init_threading();

    thread *s = thread_create("thread_sleeper", thread_sleeper);
    (void)s;

    for (int i = 0; i < 2; i++) {
        printf("hello from main thread\n");
        lock_scheduler();
        schedule();
        unlock_scheduler();
    }

    write_serial_string("Hello, host!\n");
    while (true) {
        lock_scheduler();
        schedule();
        unlock_scheduler();
        __asm__ volatile("hlt");
    }
}
