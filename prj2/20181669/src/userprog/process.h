#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
struct child* find_child(int tid, struct list *child_list_ptr);
void free_child(struct list *child_list_ptr);

#endif /* userprog/process.h */
