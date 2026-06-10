#include <stdint.h>
void page_directory_init();
void map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);
uint32_t virt_to_phys(uint32_t virtual_addr);
