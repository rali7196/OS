#include "vm/frame.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "vm/swap.h"

static struct list frame_table;
static struct lock frame_table_lock;

void frame_table_init(void) {
    list_init(&frame_table);
    lock_init(&frame_table_lock);
}

void *evict_frame(void) {   //FIFO
    struct frame_table_entry *fte;
    struct list_elem *e;
    
    lock_acquire(&frame_table_lock);
    for (e = list_begin(&frame_table); e != list_end(&frame_table); e = list_next(e)) {
        fte = list_entry(e, struct frame_table_entry, elem);
        if (fte != NULL) {
            // check if the page needs to be written to swap
            if (pagedir_is_dirty(fte->owner->pagedir, fte->vme->addr)) {
                size_t swap_index = swap_write(fte->frame);
                fte->vme->in_swap = true;
                fte->vme->swap_index = swap_index;
            }
            pagedir_clear_page(fte->owner->pagedir, fte->vme->addr);
            list_remove(e);
            void *frame_addr = fte->frame;
            free(fte);
            lock_release(&frame_table_lock);
            return frame_addr;
        }
    }
    lock_release(&frame_table_lock);
    return NULL; // No frame could be evicted
}
//
// Allocate a frame and add it to the frame table
void *allocate_frame(enum palloc_flags flags) {
    lock_acquire(&frame_table_lock);
    void *frame = palloc_get_page(flags);
    if (frame != NULL) {
        struct frame_table_entry *fte = malloc(sizeof(struct frame_table_entry));
        if (fte == NULL) {  // error catching
            palloc_free_page(frame);
            lock_release(&frame_table_lock);
            return NULL;
        }
        fte->frame = frame;
        fte->owner = thread_current();
        fte->vme = NULL;
        list_push_back(&frame_table, &fte->elem);
    }else{  // error catching
        palloc_free_page(frame);    
    }
    lock_release(&frame_table_lock);
    return frame;
}

// Free a frame and remove it from the frame table
void free_frame(void *frame) {
    lock_acquire(&frame_table_lock);
    struct list_elem *e;
    for (e = list_begin(&frame_table); e != list_end(&frame_table); e = list_next(e)) {
        struct frame_table_entry *fte = list_entry(e, struct frame_table_entry, elem);
        if (fte->frame == frame) {
            list_remove(e);
            palloc_free_page(frame);
            free(fte);
            break;
        }
    }
    lock_release(&frame_table_lock);
}

