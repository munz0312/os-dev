#include <stdint.h>

typedef enum {
    READY,
    BLOCKED,
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
} thread;

// defined in switch.S
void switch_context(uint32_t *old_esp, uint32_t *new_esp);
thread *thread_create(const char *name, void (*entry)(void));
void init_threading();
void schedule();
