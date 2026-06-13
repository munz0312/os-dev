#include <kernel/pit.h>
#include <sys/io.h>

void pit_setup() {
    // 0x36 = 0011 0110
    // 00 = channel 0
    // 11 = access mode - lobyte/hibyte
    // 011 = mode 3
    // 0 = binary mode
    outb(0x43, 0x36);
    outb(0x40, 0x9C);
    outb(0x40, 0x2E);
}
