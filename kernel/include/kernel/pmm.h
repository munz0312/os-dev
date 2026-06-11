#include "kernel/multiboot.h"
#include <stdint.h>

void pmm_init(multiboot_info_t *mbd);
int32_t pmm_alloc_frame();
void pmm_free_frame(uint32_t addr);
