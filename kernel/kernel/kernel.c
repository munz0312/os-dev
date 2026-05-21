#include <stdio.h>

#include <kernel/serial.h>
#include <kernel/tty.h>

void kernel_main(void) {
    init_serial();
    terminal_initialize();
    printf("Hello, kernel World!\n");
    write_serial('B');
}
