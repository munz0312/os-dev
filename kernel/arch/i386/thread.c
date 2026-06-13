#include <kernel/kmalloc.h>
#include <kernel/thread.h>
#include <stdint.h>
#include <string.h>

thread *thread_create(const char *name, void (*entry)(void)) {
    static uint32_t tid = 0;
    thread *new_thread = (thread *)kmalloc(sizeof(thread));
    void *stack = kmalloc(4080);

    new_thread->thread_id = tid++;
    new_thread->status = READY;
    memcpy(new_thread->name, name, strlen(name));
    new_thread->stack_alloc = (uint32_t)stack;

    uint32_t *sp = (uint32_t *)((uint32_t)stack + 4080);

    *(--sp) = (uint32_t)entry; // return address
    // push zeroed registers - ebp, ebx, esi and edi
    // are callee-saved registers
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;

    new_thread->esp = (uint32_t)sp;
    return new_thread;
}
