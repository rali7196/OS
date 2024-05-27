#ifndef SWAP_H
#define SWAP_H

#include "devices/block.h"
#include <bitmap.h>

void swap_init(void);
size_t swap_write(void *page);
void swap_read(size_t swap_index, void *page);
void swap_free(size_t swap_index);

#endif