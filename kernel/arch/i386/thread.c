#include <kernel/idt.h>
#include <kernel/kmalloc.h>
#include <kernel/thread.h>
#include <stdint.h>
#include <string.h>

int IRQ_disable_counter = 0;
int postpone_task_switches_counter = 0;
int task_switches_postponed_flag = 0;

thread *current_thread;
thread *first_thread_ready = NULL;
thread *last_thread_ready = NULL;

thread *first_sleep_thread = NULL;
thread *last_sleep_thread = NULL;

void list_append(thread **first, thread **last, thread *task) {
    task->next = NULL;
    if (*first == NULL) {
        *first = task;
        *last = task;
    } else {
        (*last)->next = task;
        *last = task;
    }
}

void check_wakeup() {
    uint64_t now = get_current_ticks();
    thread dummy;
    dummy.next = first_sleep_thread;
    thread *prev = &dummy;
    thread *curr = first_sleep_thread;

    while (curr != NULL) {
        if (curr->wake_tick <= now) {
            prev->next = curr->next;
            thread *to_wake = curr;
            curr = prev->next;

            to_wake->status = READY;
            list_append(&first_thread_ready, &last_thread_ready, to_wake);
        } else {
            prev = curr;
            curr = curr->next;
        }
    }

    // update head and tail pointers
    first_sleep_thread = dummy.next;
    if (first_sleep_thread == NULL) {
        last_sleep_thread = NULL;
    } else {
        last_sleep_thread = first_sleep_thread;
        while (last_sleep_thread->next != NULL)
            last_sleep_thread = last_sleep_thread->next;
    }
}

void nano_sleep_until(uint64_t ticks) {
    lock_scheduler();
    thread *task = current_thread;
    task->status = SLEEPING;
    task->wake_tick = ticks;
    list_append(&first_sleep_thread, &last_sleep_thread, task);
    schedule();
    unlock_scheduler();
}

void nanosleep(uint64_t ticks) {
    nano_sleep_until(get_current_ticks() + ticks);
}

void lock_scheduler() {
#ifndef SMP
    __asm__ volatile("cli");
    IRQ_disable_counter++;
    postpone_task_switches_counter++;
#endif
}

void unlock_scheduler() {
#ifndef SMP
    postpone_task_switches_counter--;
    if (postpone_task_switches_counter == 0) {
        if (task_switches_postponed_flag != 0) {
            task_switches_postponed_flag = 0;
            schedule();
        }
    }
    IRQ_disable_counter--;
    if (IRQ_disable_counter == 0) {
        __asm__ volatile("sti");
    }
#endif
}

void block_task(status new_status) {
    lock_scheduler();
    current_thread->status = new_status;
    schedule();
    unlock_scheduler();
}

void unblock_task(thread *task) {
    lock_scheduler();
    if (first_thread_ready == NULL) {
        // pre-empt the current_thread

        thread *old = current_thread;
        task->status = RUNNING;

        old->status = READY;
        list_append(&first_thread_ready, &last_thread_ready, old);

        current_thread = task;
        switch_context(&old->esp, &task->esp);
    } else {
        task->status = READY;
        list_append(&first_thread_ready, &last_thread_ready, task);
    }
    unlock_scheduler();
}

void init_threading() {
    current_thread = kmalloc(sizeof(thread));
    current_thread->thread_id = 0;
    current_thread->status = RUNNING;
    memcpy(current_thread->name, "main", 5);
    current_thread->esp = 0;
    current_thread->next = NULL;
}

void schedule() {

    if (postpone_task_switches_counter != 0) {
        task_switches_postponed_flag = 1;
        return;
    }

    if (first_thread_ready != NULL) {

        thread *old = current_thread;
        thread *task = first_thread_ready;

        first_thread_ready = task->next;
        task->status = RUNNING;
        current_thread = task;

        // re-add the descheduled thread only if it was not blocked/paused or
        // something like that
        if (old->status == RUNNING) {
            old->status = READY;
            list_append(&first_thread_ready, &last_thread_ready, old);
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
    list_append(&first_thread_ready, &last_thread_ready, new_thread);
    unlock_scheduler();

    return new_thread;
}
