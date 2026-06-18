#include <stdint.h>

typedef enum {
    READY,
    BLOCKED,
    SLEEPING,
    PAUSED,
    RUNNING,
    DEAD,
} status;

typedef struct thread {
    uint32_t thread_id;
    status status;
    char name[16];
    uint32_t esp;
    uint32_t stack_alloc;
    struct thread *next;
    uint64_t wake_tick;
} thread;

// defined in switch.S. please lock the scheduler before calling
void switch_context(uint32_t *old_esp, uint32_t *new_esp);
thread *thread_create(const char *name, void (*entry)(void));
void init_threading();
// please lock the scheduler before calling
void schedule();
void lock_scheduler();
void unlock_scheduler();
void block_task(status new_status);
void unblock_task(thread *task);
void nanosleep(uint64_t ticks);
void nano_sleep_until(uint64_t ticks);
void check_wakeup();
