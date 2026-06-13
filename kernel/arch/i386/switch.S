.global switch_context
switch_context:
    push %ebp
    push %ebx
    push %esi
    push %edi
    
    mov 20(%esp), %eax
    mov %esp, (%eax)
    mov 24(%esp), %eax
    mov (%eax), %esp

    pop %edi
    pop %esi
    pop %ebx
    pop %ebp

    ret
    
    
