.section .text
.global gdt_flush
.type gdt_flush, @function
gdt_flush:
    movl 4(%esp), %eax
    lgdt (%eax)
    ljmp $0x08, $.complete_flush

.complete_flush:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    ret
