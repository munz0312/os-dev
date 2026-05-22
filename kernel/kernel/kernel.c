#include <kernel/gdt.h>
#include <kernel/serial.h>
#include <kernel/tty.h>
#include <stddef.h>
#include <stdio.h>

void kernel_main(void) {
    init_serial();
    populate_gdt();
    terminal_initialize();
    printf("Hello, kernel World!\n");
    write_serial_string("Hello, host!\n");
}
