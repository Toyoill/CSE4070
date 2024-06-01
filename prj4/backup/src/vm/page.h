#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <inttypes.h>
#include <hash.h>
#include <debug.h>

#include "threads/thread.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "filesys/file.h"

struct page_table_entry {
    struct file *file;
    off_t ofs;
    uint8_t *upage;
    uint8_t *kpage;
    size_t page_read_bytes;
    size_t page_zero_bytes;
    bool readonly;
    bool loaded;
    size_t swap_idx;

    struct hash_elem h_elem;
};

void page_init_table (struct hash** page_table);
bool page_copy_table (struct hash* from, struct hash* to);
struct page_table_entry* page_create_and_insert_entry (struct hash* page_table, struct file *file, off_t ofs, uint8_t *upage,
    size_t page_read_bytes, size_t page_zero_bytes, bool read_only);
struct page_table_entry* page_entry (struct file *file, off_t ofs, uint8_t *upage,
    size_t page_read_bytes, size_t page_zero_bytes, bool read_only);
bool page_fault_handler (void* fault_addr, bool write);

hash_action_func page_free_entry;

unsigned page_hash (const struct hash_elem *p_, void *aux UNUSED);
bool page_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED);
struct page_table_entry* page_lookup(struct hash* page_table, const void *upage);

#endif