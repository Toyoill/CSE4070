#include "userprog/syscall.h"

#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>

#include "devices/input.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "process.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"

struct lock filesys_lock;

static void syscall_handler(struct intr_frame *);
static int check_addr_validity(const void *uaddr);
int sys_wait(int tid);
void sys_exit(int status);
tid_t sys_exec(const char *fname);

static int check_addr_validity(const void *uaddr) {
  if (!uaddr) return -1;
  if (!is_user_vaddr(uaddr)) return -1;
  if (!pagedir_get_page(thread_current()->pagedir, uaddr)) return -1;
  return 0;
}

void syscall_init(void) {
  lock_init(&filesys_lock);
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void syscall_handler(struct intr_frame *f) {
  void *esp = f->esp;
  int syscall_num;

  if (check_addr_validity(esp) == -1) sys_exit(-1);
  syscall_num = *(int *)esp;

  if (syscall_num == SYS_WRITE) {
    int fd, size, buffer;

    if (check_addr_validity(esp + 4) == -1) sys_exit(-1);
    fd = *(int *)(esp + 4);
    if (check_addr_validity(esp + 8) == -1) sys_exit(-1);
    buffer = *(int *)(esp + 8);
    if (check_addr_validity(esp + 12) == -1) sys_exit(-1);
    size = *(int *)(esp + 12);

    if (fd < 0 || 128 < fd) {
      f->eax = -1;
      return;
    }

    f->eax = sys_write(fd, (void *)buffer, (unsigned)size);

  } else if (syscall_num == SYS_READ) {
    int fd, size, buffer;

    if (check_addr_validity(esp + 4) == -1) sys_exit(-1);
    fd = *(int *)(esp + 4);
    if (check_addr_validity(esp + 8) == -1) sys_exit(-1);
    buffer = *(int *)(esp + 8);
    if (check_addr_validity(esp + 12) == -1) sys_exit(-1);
    size = *(int *)(esp + 12);

    if (fd < 0 || 128 < fd) {
      f->eax = -1;
      return;
    }

    f->eax = sys_read(fd, (void *)buffer, (unsigned)size);

  } else if (syscall_num == SYS_EXEC) {
    char *fname;

    if (check_addr_validity(esp + 4) == -1) sys_exit(-1);
    fname = *(char **)(esp + 4);

    f->eax = sys_exec((const char *)fname);

  } else if (syscall_num == SYS_EXIT) {
    int status;

    if (check_addr_validity(esp + 4) == -1) sys_exit(-1);
    status = *(int *)(esp + 4);

    sys_exit(status);

  } else if (syscall_num == SYS_WAIT) {
    int tid;

    if (check_addr_validity(esp + 4) == -1) sys_exit(-1);
    tid = *(int *)(esp + 4);

    f->eax = sys_wait(tid);
  } else if (syscall_num == SYS_HALT) {
    shutdown_power_off();
  } else if (syscall_num == SYS_FIBO) {
    int n;

    check_addr_validity(esp + 4);
    n = *(int *)(esp + 4);

    f->eax = sys_fibonacci(n);
  } else if (syscall_num == SYS_MAX_FOUR) {
    int a, b, c, d;

    if (check_addr_validity(esp + 4) == -1) sys_exit(-1);
    a = *(int *)(esp + 4);
    if (check_addr_validity(esp + 8) == -1) sys_exit(-1);
    b = *(int *)(esp + 8);
    if (check_addr_validity(esp + 12) == -1) sys_exit(-1);
    c = *(int *)(esp + 12);
    if (check_addr_validity(esp + 16) == -1) sys_exit(-1);
    d = *(int *)(esp + 16);

    f->eax = sys_max_of_four_int(a, b, c, d);
  } else if (syscall_num == SYS_CREATE) {
    if (check_addr_validity(esp + 4) == -1) sys_exit(-1);
    const char *file = *(const char **)(esp + 4);
    if (check_addr_validity(esp + 8) == -1) sys_exit(-1);
    unsigned initial_size = *(unsigned *)(esp + 8);

    f->eax = sys_create(file, initial_size);
  } else if (syscall_num == SYS_REMOVE) {
    if (check_addr_validity(esp + 4) == -1) sys_exit(-1);
    const char *file = *(const char **)(esp + 4);

    f->eax = sys_remove(file);
  } else if (syscall_num == SYS_OPEN) {
    if (check_addr_validity(esp + 4) == -1) sys_exit(-1);
    const char *file = *(const char **)(esp + 4);

    f->eax = sys_open(file);
  } else if (syscall_num == SYS_CLOSE) {
    if (check_addr_validity(esp + 4) == -1) sys_exit(-1);
    int fd = *(int *)(esp + 4);

    sys_close(fd);
  } else if (syscall_num == SYS_FILESIZE) {
    if (check_addr_validity(esp + 4) == -1) sys_exit(-1);
    int fd = *(int *)(esp + 4);

    f->eax = sys_filesize(fd);
  } else if (syscall_num == SYS_SEEK) {
    if (check_addr_validity(esp + 4) == -1) sys_exit(-1);
    int fd = *(int *)(esp + 4);
    if (check_addr_validity(esp + 8) == -1) sys_exit(-1);
    unsigned position = *(unsigned *)(esp + 8);

    sys_seek(fd, position);
  } else if (syscall_num == SYS_TELL) {
    if (check_addr_validity(esp + 4) == -1) sys_exit(-1);
    int fd = *(int *)(esp + 4);

    f->eax = sys_tell(fd);
  }
}

int sys_write(int fd, const char *buffer, unsigned size) {
  if (check_addr_validity(buffer) == -1) sys_exit(-1);
  lock_acquire(&filesys_lock);

  if (fd == 1) {
    // STDOUT
    putbuf(buffer, size);
    lock_release(&filesys_lock);
    return size;
  } else {
    struct thread *th = thread_current();
    struct file *f;

    if (fd < 0 || 127 < fd) return -1;
    if (!(f = th->file_descriptor[fd])) {
      lock_release(&filesys_lock);
      return -1;
    }

    int t = file_write(f, (const void *)buffer, size);
    lock_release(&filesys_lock);
    return t;
  }
}

int sys_read(int fd, char *buffer, unsigned length) {
  if (check_addr_validity(buffer) == -1) sys_exit(-1);
  lock_acquire(&filesys_lock);

  if (fd == 0) {
    // STDIN
    unsigned cnt = 0;
    uint8_t c;
    for (cnt = 0; cnt < length; cnt++) {
      c = input_getc();
      buffer[cnt] = c;
      if (!c) break;
    }
    lock_release(&filesys_lock);
    return length - cnt;
  } else {
    struct thread *th = thread_current();
    struct file *f;
    int ret;

    if (!(f = th->file_descriptor[fd])) {
      lock_release(&filesys_lock);
      return -1;
    }
    ret = file_read(f, (void *)buffer, length);

    lock_release(&filesys_lock);
    return ret;
  }

  return 0;
}

int sys_wait(int tid) { return process_wait(tid); }

void sys_exit(int status) {
  printf("%s: exit(%d)\n", thread_name(), status);

  struct thread *cur = thread_current();
  free_child(&cur->child_list);
  for (int fd = 0; fd < 128; fd++)
    if (cur->file_descriptor[fd]) file_close(cur->file_descriptor[fd]);

  struct child *ch = find_child(cur->tid, &cur->parent->child_list);
  ch->exit_status = status;
  thread_exit();
}

tid_t sys_exec(const char *fname) {
  if (check_addr_validity(fname) == -1) return -1;

  return process_execute(fname);
}

int sys_fibonacci(int n) {
  if (n == 0)
    return 0;
  else if (n == 1)
    return 1;
  else if (n == 2)
    return 1;
  else {
    int arr[3] = {0, 1, 1}, k = 2;
    while (k++ < n) {
      arr[0] = arr[1];
      arr[1] = arr[2];
      arr[2] = arr[0] + arr[1];
    }
    return arr[2];
  }
}

int sys_max_of_four_int(int a, int b, int c, int d) {
  int ret = a;
  ret = ret > b ? ret : b;
  ret = ret > c ? ret : c;
  ret = ret > d ? ret : d;

  return ret;
}

int sys_create(const char *file, unsigned initial_size) {
  if (check_addr_validity(file) == -1) sys_exit(-1);
  if (!strlen(file)) return 0;
  return filesys_create(file, initial_size);
}

int sys_remove(const char *file) {
  if (check_addr_validity(file) == -1) return 0;
  lock_acquire(&filesys_lock);
  int ret = filesys_remove(file);
  lock_release(&filesys_lock);
  return ret;
}

int sys_open(const char *file) {
  if (!file) return -1;
  lock_acquire(&filesys_lock);

  int fd;
  struct thread *th = thread_current();
  struct file *f;

  for (fd = 3; fd < 128 && th->file_descriptor[fd]; fd++)
    ;
  if (fd == 128 || !(f = filesys_open(file))) {
    lock_release(&filesys_lock);
    return -1;
  }

  if (!strcmp(file, th->name)) file_deny_write(f);

  th->file_descriptor[fd] = f;
  lock_release(&filesys_lock);
  return fd;
}

int sys_filesize(int fd) {
  struct thread *th = thread_current();
  struct file *f;

  if (!(f = th->file_descriptor[fd])) return -1;
  return file_length(f);
}

void sys_seek(int fd, unsigned position) {
  struct thread *th = thread_current();
  struct file *f;

  if (!(f = th->file_descriptor[fd])) return;
  file_seek(f, position);
}

unsigned sys_tell(int fd) {
  struct thread *th = thread_current();
  struct file *f;

  if (!(f = th->file_descriptor[fd])) return 0;
  return file_tell(f);
}

void sys_close(int fd) {
  if (fd < 0 || 127 < fd) return;

  lock_acquire(&filesys_lock);

  struct thread *th = thread_current();
  struct file *f;

  if (!(f = th->file_descriptor[fd])) {
    lock_release(&filesys_lock);
    return;
  }
  file_close(f);

  th->file_descriptor[fd] = NULL;

  lock_release(&filesys_lock);
}