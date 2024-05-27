#ifndef PAGE_H
#define PAGE_H


#include <hash.h>
#include <stdbool.h>
#include <debug.h>
#include "filesys/file.h"
#include "threads/synch.h"
#include "lib/kernel/hash.h"

struct vm_entry {
  //add all fields outlined in 
  struct hash_elem hash_elem;
  void* addr;
  char permissions;
  bool in_memory;
  bool in_swap;
  bool in_file;
  size_t swap_index;
  struct file* file;
  off_t offset;
  size_t read_bytes;
  size_t zero_bytes;
  bool writable;

  //TODO: add additional information for arguments for load_segment, block r/w
};


unsigned page_hash (const struct hash_elem *p_, void *aux);
bool page_less(const struct hash_elem *a_, const struct hash_elem *b_, void *aux);
void spt_init(struct hash *spt);

//TO IMPLEMENT
//frees supplemental page table
void spt_destroy(struct hash *spt);
//identifies a vm_entry struct in hash table given vaddr
struct vm_entry* spt_find_entry(struct hash *spt, void* vaddr);
//inserts a vm_entry struct into hash table spt
void spt_insert_entry(struct hash *spt, struct vm_entry *vme UNUSED);
//deletes a vm_entry from hash table spt
void spt_delete_entry(struct hash *spt, struct vm_entry *vme UNUSED);

#endif