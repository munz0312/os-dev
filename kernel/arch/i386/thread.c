#include <kernel/kmalloc.h>
#include <kernel/thread.h>
#include <stdint.h>
#include <string.h>

int IRQ_disable_counter = 0;

void lock_scheduler() {
#ifndef SMP
    __asm__ volatile("cli");
    IRQ_disable_counter++;
#endif
}

void unlock_scheduler() {
#ifndef SMP
    IRQ_disable_counter--;
    if (IRQ_disable_counter == 0) {
        __asm__ volatile("sti");
    }
#endif
}

thread *current_thread;
thread *first_thread_ready = NULL;
thread *last_thread_ready = NULL;

void init_threading() {
    current_thread = kmalloc(sizeof(thread));
    current_thread->thread_id = 0;
    current_thread->status = RUNNING;
    memcpy(current_thread->name, "main", 5);
    current_thread->esp = 0;
    current_thread->next = NULL;
}

void schedule() {
    if (first_thread_ready != NULL) {

        thread *old = current_thread;
        thread *task = first_thread_ready;

        first_thread_ready = task->next;
        task->status = RUNNING;
        current_thread = task;

        // re-add the descheduled thread
        old->status = READY;
        old->next = NULL;
        if (first_thread_ready == NULL) {
            first_thread_ready = old;
            last_thread_ready = old;
        } else {
            last_thread_ready->next = old;
            last_thread_ready = old;
        }

        switch_context(&old->esp, &task->esp);
    }
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

    lock_scheduler();
    new_thread->next = NULL;
    if (first_thread_ready == NULL) {
        first_thread_ready = new_thread;
        last_thread_ready = new_thread;
    } else {
        last_thread_ready->next = new_thread;
        last_thread_ready = new_thread;
    }
    unlock_scheduler();

    return new_thread;
}
