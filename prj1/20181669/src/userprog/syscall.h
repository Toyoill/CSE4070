#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
void sys_exit(int status);

/* Project 2 additional system call*/
int sys_fibonacci(int n);
int sys_max_of_four_int(int a, int b, int c, int d);

#endif /* userprog/syscall.h */
