
#ifndef FRAME_H
#define FRAME_H

#include "threads/synch.h"
#include "threads/palloc.h"
#include "list.h"
#include "hash.h"
#include "vm/page.h"
#include "vm/swap.h"

/*
* Frame table entry
* Contains the paddr/owner/corresponding spte/list element of the frame
*/
struct frame_table_entry {
    void *frame;  // physical address of the frame
    struct thread *owner; // pointer to the owner of the frame, for eviction
    struct vm_entry *vme; // pointer to the corresponding spte
    struct list_elem elem; // list element for the frame list
};

void frame_table_init(void);
void *allocate_frame(enum palloc_flags flags);
void free_frame(void *frame);
void *evict_frame(void);

#endif