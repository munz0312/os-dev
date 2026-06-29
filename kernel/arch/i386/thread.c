#include <kernel/idt.h>
#include <kernel/kmalloc.h>
#include <kernel/thread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int IRQ_disable_counter = 0;
int postpone_task_switches_counter = 0;
int task_switches_postponed_flag = 0;

uint64_t ticks_since_boot = 0;

thread *idle_task;
thread *current_thread;
thread *first_thread_ready = NULL;
thread *last_thread_ready = NULL;

thread *first_sleep_thread = NULL;
thread *last_sleep_thread = NULL;

void kernel_idle_task(void) {
    unlock_scheduler();
    printf("entered idle_task");
    while (true) {
        __asm__ volatile("hlt");
    }
}
void switch_context_wrapper(thread *old, thread *new) {
    if (postpone_task_switches_counter != 0) {
        task_switches_postponed_flag = 1;
        return;
    }

    switch_context(&old->esp, &new->esp);
}

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

    if (ticks < get_current_ticks()) {
        unlock_scheduler();
        return;
    }

    thread *task = current_thread;
    task->status = SLEEPING;
    task->wake_tick = ticks;
    list_append(&first_sleep_thread, &last_sleep_thread, task);
    unlock_scheduler();
    block_task(SLEEPING);
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
    if ((first_thread_ready == NULL) || (current_thread == idle_task)) {
        // pre-empt the current_thread

        thread *old = current_thread;
        task->status = RUNNING;

        old->status = READY;
        list_append(&first_thread_ready, &last_thread_ready, old);

        current_thread = task;
        switch_context_wrapper(old, task);
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

    idle_task = thread_create("idle_task", kernel_idle_task);
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

        if (task == idle_task) {
            if (first_thread_ready != NULL) {
                // idle was selected but other tasks are ready - pick the next
                // one
                task = first_thread_ready;
                first_thread_ready = task->next;
                list_append(&first_thread_ready, &last_thread_ready, idle_task);
            } else if (current_thread->status == RUNNING) {
                // no other tasks ready, current can keep running
                list_append(&first_thread_ready, &last_thread_ready, idle_task);
                return;
            } else {
                // idle is the only option
            }
        }

        task->status = RUNNING;
        current_thread = task;

        // re-add the descheduled thread only if it was not blocked/paused or
        // something like that
        if (old->status == RUNNING) {
            old->status = READY;
            list_append(&first_thread_ready, &last_thread_ready, old);
        }

        switch_context_wrapper(old, task);
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
