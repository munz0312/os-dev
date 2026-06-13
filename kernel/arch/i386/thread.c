#include <kernel/kmalloc.h>
#include <kernel/thread.h>
#include <stdint.h>
#include <string.h>

thread *current_thread;

void init_threading() {
    current_thread = kmalloc(sizeof(thread));
    current_thread->thread_id = 0;
    current_thread->status = RUNNING;
    memcpy(current_thread->name, "main", 5);
    current_thread->esp = 0;
    current_thread->next = current_thread;
}

void schedule() {
    thread *old = current_thread;
    thread *next = current_thread->next;
    old->status = READY;
    next->status = RUNNING;
    current_thread = next;
    switch_context(&old->esp, &next->esp);
}

thread *thread_create(const char *name, void (*entry)(void)) {
    static uint32_t tid = 0;
    thread *new_thread = (thread *)kmalloc(sizeof(thread));
    void *stack = kmalloc(4080);

    new_thread->thread_id = ++tid;
    new_thread->status = READY;
    memcpy(new_thread->name, name, strlen(name) + 1);
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
    new_thread->next = current_thread->next;
    current_thread->next = new_thread;
    return new_thread;
}
