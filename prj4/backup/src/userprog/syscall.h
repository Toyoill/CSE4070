#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
void sys_exit(int status);

/* Project 1 additional system call*/
int sys_fibonacci(int n);
int sys_max_of_four_int(int a, int b, int c, int d);

/* Project 2 */
int sys_create (const char *file, unsigned initial_size);
int sys_remove (const char *file);
int sys_open (const char *file);
void sys_close (int fd);
int sys_filesize (int fd);
int sys_read (int fd, char *buffer, unsigned length);
int sys_write (int fd, const char *buffer, unsigned length);
void sys_seek (int fd, unsigned position);
unsigned sys_tell (int fd);

#endif /* userprog/syscall.h */
