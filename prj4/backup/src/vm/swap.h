#ifndef VM_SWAP_H
#define VM_SWAP_H

#include <bitmap.h>
#include "userprog/pagedir.h"
#include "devices/block.h"
#include "vm/page.h"
#include "vm/frame.h"
#include "threads/vaddr.h"

struct block *swap_block;
struct bitmap *swap_table;

#define SECTOR_PER_PAGE (PGSIZE / BLOCK_SECTOR_SIZE)

void init_swap_table(void);
bool swap_out(struct page_table_entry* pte);
bool swap_in(struct page_table_entry* pte);
void free_swap_table(void);

#endif