#include "swap.h"
#include <debug.h>
#include "threads/synch.h"
#include "threads/malloc.h"

struct lock swap_tb_lock, swap_blk_lock;

void init_swap_table(void){
    swap_block = block_get_role (BLOCK_SWAP);
    if(!swap_block) {
        return;
    }

    swap_table = bitmap_create (block_size (swap_block) / SECTOR_PER_PAGE);
    lock_init(&swap_tb_lock);
    lock_init(&swap_blk_lock);
}

bool swap_out(struct page_table_entry* pte){
    void* kpage = pte->kpage;
    size_t idx = bitmap_scan_and_flip (swap_table, 0, 1, false);
    if (idx == BITMAP_ERROR) {
        return false;
    }

    int mapping = idx * SECTOR_PER_PAGE;
    for (int i = 0; i < SECTOR_PER_PAGE; i++)
        block_write (swap_block, mapping + i, kpage + i * PGSIZE);
    
    pte->swap_idx = idx;

    pagedir_clear_page (thread_current()->pagedir, pte->upage);
    
    return true;
}

bool swap_in(struct page_table_entry* pte){
    void* kpage = pte->kpage;

    size_t idx = pte->swap_idx;
    ASSERT(bitmap_test(swap_table, idx) == true);

    bitmap_set(swap_table, idx, false);
    int mapping = idx * SECTOR_PER_PAGE;
    for (int i = 0; i < SECTOR_PER_PAGE; i++)
        block_read (swap_block, mapping + i, kpage + i * PGSIZE);
    
    return true;
}

void free_swap_table(void){
    bitmap_destroy(swap_table);
}