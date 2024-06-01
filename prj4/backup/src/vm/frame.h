#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <hash.h>
#include <debug.h>
#include "threads/palloc.h"
#include "threads/synch.h"

struct frame_entry {
    void *page_ptr;
    struct hash_elem h_elem;
    struct page_table_entry* pte;
};

struct hash frame_table;
struct lock frame_table_lock;

void frame_init(void);
struct frame_entry* frame_get_page(enum palloc_flags flag);
void frame_free_page(void *ptr);

hash_action_func frame_free_entry;

unsigned frame_hash (const struct hash_elem *p_, void *aux UNUSED);
bool frame_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED);
struct frame_entry* frame_lookup(const void *ptr);

#endif