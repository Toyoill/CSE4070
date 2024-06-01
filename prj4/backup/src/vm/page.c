#include "page.h"
#include <string.h>
#include "threads/malloc.h"
#include "vm/frame.h"
#include "vm/swap.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"

bool install_page(void *upage, void *kpage, bool writable);

/*  functions for building hash table */

unsigned page_hash (const struct hash_elem *p_, void *aux UNUSED){
    const struct page_table_entry *p = hash_entry (p_, struct page_table_entry, h_elem);
    return hash_bytes(&p->upage, sizeof p->upage);
}

bool page_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED){
    const struct page_table_entry *a = hash_entry (a_, struct page_table_entry, h_elem);
    const struct page_table_entry *b = hash_entry (b_, struct page_table_entry, h_elem);
    return a->upage < b->upage;
}

struct page_table_entry* page_lookup(struct hash* page_table, const void *upage){
    ASSERT (pg_ofs (upage) == 0);

    struct page_table_entry pte;
    struct hash_elem *e;

    pte.upage = (void *) upage;
    e = hash_find(page_table, &pte.h_elem);
    return e != NULL ? hash_entry (e, struct page_table_entry, h_elem) : NULL;
}

/*      end     */

void page_init_table (struct hash** page_table){
    *page_table = malloc(sizeof (struct hash));
    hash_init(*page_table, page_hash, page_less, NULL);
}

struct page_table_entry* page_create_and_insert_entry (struct hash* page_table, struct file *file, off_t ofs, uint8_t *upage,
    size_t page_read_bytes, size_t page_zero_bytes, bool read_only){
    
    struct page_table_entry *pte = page_entry (file, ofs, upage, page_read_bytes, 
        page_zero_bytes, read_only);

    if (!hash_insert(page_table, &pte->h_elem)) return pte;
    return NULL;
}

struct page_table_entry* page_entry (struct file *file, off_t ofs, uint8_t *upage,
    size_t page_read_bytes, size_t page_zero_bytes, bool read_only){
    upage = pg_round_down((const void *)upage);

    struct page_table_entry* pte = malloc (sizeof (struct page_table_entry));
    pte->file = file;
    pte->ofs = ofs;
    pte->upage = upage;
    pte->kpage = NULL;
    pte->page_read_bytes = page_read_bytes;
    pte->page_zero_bytes = page_zero_bytes;
    pte->readonly = read_only;
    pte->loaded = false;

    return pte;
}

bool page_fault_handler (void* fault_addr, bool write) {
    struct frame_entry* frame;
    uint8_t *kpage;

    fault_addr = pg_round_down((const void *)fault_addr);

    // ASSERT (pg_ofs (fault_addr) == 0);
    struct hash *page_table = thread_current() -> page_table;
    struct page_table_entry *pte = page_lookup(page_table, (const void *)fault_addr);
    
    if(!pte || (pte->readonly && write)) return false;
    frame = frame_get_page(PAL_USER);
    frame->pte = pte;
    kpage = frame->page_ptr;

    if (!kpage) return false;

    if(pte->loaded){
        swap_in (pte);
    } else{
        if(pte->file){
            file_seek(pte->file, pte->ofs);
            if (file_read(pte->file, kpage, pte->page_read_bytes) != (int)pte->page_read_bytes) {
                frame_free_page(kpage);
                return false;
            }
            memset(kpage + pte->page_read_bytes, 0, pte->page_zero_bytes);
        } 
        pte->loaded = true;
    }

    if (!install_page(fault_addr, kpage, !pte->readonly)) {
      frame_free_page(kpage);
      return false;
    }
    pte->kpage = kpage;
    return true;
}

bool page_copy_table (struct hash* from, struct hash* to){

    ASSERT(from != NULL);
    ASSERT(to != NULL);
    
    hash_clear(to, page_free_entry);
    struct hash_iterator i;
    struct page_table_entry *pte, *tmp;
    hash_first (&i, from);
    while (hash_next (&i)){
        pte = hash_entry (hash_cur (&i), struct page_table_entry, h_elem);
        tmp = page_entry (pte->file, pte->ofs, pte->upage, pte->page_read_bytes, 
            pte->page_zero_bytes, pte->readonly);
        hash_insert(to, &tmp->h_elem);
    }

    return true;
}

void page_free_entry (struct hash_elem *element, void *aux UNUSED){
    struct page_table_entry* pte = hash_entry (element, struct page_table_entry, h_elem);
    free(pte);
}

bool install_page(void *upage, void *kpage, bool writable) {
  struct thread *t = thread_current();


  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (pagedir_get_page(t->pagedir, upage) == NULL &&
          pagedir_set_page(t->pagedir, upage, kpage, writable));
}
