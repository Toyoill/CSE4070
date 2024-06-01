#include "threads/malloc.h"
#include "threads/thread.h"
#include "frame.h"
#include "threads/vaddr.h"
#include "swap.h"

void frame_init(){
    lock_init(&frame_table_lock);
    hash_init(&frame_table, frame_hash, frame_less, NULL);
}

struct frame_entry* frame_get_page(enum palloc_flags flag){
    struct thread* t = thread_current();
    uint32_t *pd = t->pagedir;
    void* paddr;

    ASSERT(flag & PAL_USER);

    if((paddr = palloc_get_page(flag)) == NULL){
        int min_lvl = 4;
        struct frame_entry *target;
        struct hash_iterator i;
        
        hash_first (&i, &frame_table);
        while (hash_next (&i)) {
            struct frame_entry *f = hash_entry (hash_cur (&i), struct frame_entry, h_elem);
            bool accessed = pagedir_is_accessed (pd, f->page_ptr);
            bool dirty = pagedir_is_accessed (pd, f->page_ptr);
            int lvl = (accessed << 1) + dirty;
            
            if (!lvl) {
                swap_out (f->pte);
                return f;
            }
            else if (lvl < min_lvl){
                min_lvl = lvl;
                target = f;
            }
        }
        swap_out (target->pte);
        return target;

    }

    struct frame_entry *f = malloc(sizeof (struct frame_entry));
    f->page_ptr = paddr;
    hash_insert(&frame_table, &f->h_elem);

    return f;
}

void frame_free_page(void *ptr){
    ASSERT(ptr);
    
    struct frame_entry f, *del;
    struct hash_elem *e;

    f.page_ptr = ptr;

    e = hash_delete(&frame_table, &f.h_elem);

    if(e) {
        del = hash_entry(e, struct frame_entry, h_elem);
        free(del);
    }
    palloc_free_page (ptr);
}

unsigned frame_hash (const struct hash_elem *p_, void *aux UNUSED){
    const struct frame_entry *p = hash_entry (p_, struct frame_entry, h_elem);
    return hash_bytes(&p->page_ptr, sizeof p->page_ptr);
}

bool frame_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED){
    const struct frame_entry *a = hash_entry(a_, struct frame_entry, h_elem);
    const struct frame_entry *b = hash_entry(b_, struct frame_entry, h_elem);
    return a->page_ptr < b->page_ptr;
}

struct frame_entry* frame_lookup(const void *ptr){
    struct frame_entry f;
    struct hash_elem *e;

    f.page_ptr = (void *)ptr;
    e = hash_find(&frame_table, &f.h_elem);
    return e != NULL ? hash_entry (e, struct frame_entry, h_elem) : NULL;
}

void frame_free_entry (struct hash_elem *element, void *aux UNUSED){
    struct frame_entry* pte = hash_entry (element, struct frame_entry, h_elem);
    palloc_free_page(pte->page_ptr);
    free(pte);
}