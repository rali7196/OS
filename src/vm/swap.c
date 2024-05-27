#include "swap.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include <bitmap.h>
#include "devices/block.h"

#define SECTORS_PER_PAGE (PGSIZE / BLOCK_SECTOR_SIZE)

static struct block *swap_device;
static struct bitmap *swap_table;
static struct lock swap_lock;

// Initialize the swap device and data structures
void swap_init(void) {
    swap_device = block_get_role(BLOCK_SWAP);
    size_t swap_size = block_size(swap_device) / SECTORS_PER_PAGE;
    swap_table = bitmap_create(swap_size);
    bitmap_set_all(swap_table, false);  // Initially, all swap slots are free
    lock_init(&swap_lock);
}

// find free spot and write page, return the index of swap slot used
size_t swap_write(void *page) {
    lock_acquire(&swap_lock);
    size_t swap_index = bitmap_scan_and_flip(swap_table, 0, 1, false);
    if (swap_index == BITMAP_ERROR) {
        PANIC("Out of swap space");
    }

    for (int i = 0; i < SECTORS_PER_PAGE; i++) {
        block_write(swap_device, swap_index * SECTORS_PER_PAGE + i, 
                    page + i * BLOCK_SECTOR_SIZE);
    }

    lock_release(&swap_lock);
    return swap_index;
}

// load swap content at swap index into page
void swap_read(size_t swap_index, void *page) {
    lock_acquire(&swap_lock);
    for (int i = 0; i < SECTORS_PER_PAGE; i++) {
        block_read(swap_device, swap_index * SECTORS_PER_PAGE + i, 
                   page + i * BLOCK_SECTOR_SIZE);
    }
    bitmap_flip(swap_table, swap_index);
    lock_release(&swap_lock);
}

// mark a swap slot a free
void swap_free(size_t swap_index) {
    lock_acquire(&swap_lock);
    bitmap_set(swap_table, swap_index, false);
    lock_release(&swap_lock);
}