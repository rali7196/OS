#include "page.h"
#include <debug.h>
#include "threads/malloc.h"
#include "threads/thread.h"

static void spt_destroy_helper(struct hash_elem *e, void *aux UNUSED);

unsigned page_hash (const struct hash_elem *p_, void *aux UNUSED){
    const struct vm_entry *p = hash_entry(p_, struct vm_entry, hash_elem);
    return hash_bytes(&p->addr, sizeof p->addr);
}

bool page_less(const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED){
    const struct vm_entry *a = hash_entry(a_, struct vm_entry, hash_elem);
    const struct vm_entry *b = hash_entry(b_, struct vm_entry, hash_elem);
    return a->addr < b->addr;
}

void spt_init(struct hash *spt){
    hash_init(spt, page_hash, page_less, NULL);
}

void spt_destroy_helper(struct hash_elem *e, void* aux UNUSED){
  //converts the hash elem to a normal element
  //free on the normal element
  struct vm_entry *to_delete = hash_entry(e, struct vm_entry, hash_elem);
  free(to_delete);
}
//
void spt_destroy(struct hash *spt){
    hash_destroy(spt, spt_destroy_helper);
    free(spt);
}

struct vm_entry* spt_find_entry(struct hash *spt, void* vaddr){
    struct vm_entry p;
    struct hash_elem *e;

    p.addr = vaddr;
    e = hash_find( (spt), &p.hash_elem);
    return e != NULL? hash_entry(e, struct vm_entry, hash_elem) : NULL;
}

void spt_insert_entry(struct hash *spt, struct vm_entry *vme){
    hash_insert(spt, &vme->hash_elem);
}

void spt_delete_entry(struct hash *spt, struct vm_entry *vme){
    struct hash_elem *e = hash_find(spt, &vme->hash_elem);
    if (e) {
        struct vm_entry *found_vme = hash_entry(e, struct vm_entry, hash_elem);
        hash_delete(spt, e);
        free(found_vme);
    }
    // struct vm_entry p;
    // struct hash_elem *e;
    // p.addr = vme->addr;
    // e = hash_find(spt, &p.hash_elem);
    // hash_delete(spt, e);
}
