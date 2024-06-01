#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "threads/vaddr.h"
#include "devices/input.h"
#include "process.h"
#include "devices/shutdown.h"

static void syscall_handler (struct intr_frame *);
static void check_addr_validity (const void *uaddr);
void sys_write(int fd, const void *buffer, unsigned size);
int sys_read(int fd, char *buffer, int size);
int sys_wait(int tid);
void sys_exit(int status);
tid_t sys_exec(const char* fname);


static void check_addr_validity (const void *uaddr){
  if(!uaddr) sys_exit(-1);
  if(!is_user_vaddr(uaddr)) sys_exit(-1);
  if(!pagedir_get_page(thread_current()->pagedir, uaddr)) sys_exit(-1);
}

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  void* esp = f -> esp;
  int syscall_num;

  check_addr_validity(esp);
  syscall_num = *(int *)esp;

  if(syscall_num == SYS_WRITE){
    int fd, size, buffer;

    check_addr_validity(esp + 4);
    fd = *(int *)(esp + 4);
    check_addr_validity(esp + 8);
    buffer = *(int *)(esp + 8);
    check_addr_validity(esp + 12);
    size = *(int *)(esp + 12);

    sys_write(fd, (void *)buffer, (unsigned)size);
    f->eax = size;

  } else if(syscall_num == SYS_READ){

    int fd, size, buffer;

    check_addr_validity(esp + 4);
    fd = *(int *)(esp + 4);
    check_addr_validity(esp + 8);
    buffer = *(int *)(esp + 8);
    check_addr_validity(esp + 12);
    size = *(int *)(esp + 12);

    f->eax = sys_read(fd, (void *)buffer, (unsigned)size);

  } else if(syscall_num == SYS_EXEC){
      char* fname;

      check_addr_validity(esp + 4);
      fname = *(char **)(esp + 4);

      f->eax = sys_exec((const char *)fname);

  } else if(syscall_num == SYS_EXIT){
    int status;

    check_addr_validity(esp + 4);
    status = *(int *)(esp + 4);

    sys_exit(status);

  } else if(syscall_num == SYS_WAIT){
    int tid;

    check_addr_validity(esp + 4);
    tid = *(int *)(esp + 4);

    f->eax = sys_wait(tid);
  } else if(syscall_num == SYS_HALT){
    shutdown_power_off();
  } else if(syscall_num == SYS_FIBO){
    int n;

    check_addr_validity(esp + 4);
    n = *(int *)(esp + 4);

    f->eax = sys_fibonacci(n);
  } else if(syscall_num == SYS_MAX_FOUR){
    int a, b, c, d;
    
    check_addr_validity(esp + 4);
    a = *(int *)(esp + 4);
    check_addr_validity(esp + 8);
    b = *(int *)(esp + 8);
    check_addr_validity(esp + 12);
    c = *(int *)(esp + 12);
    check_addr_validity(esp + 16);
    d = *(int *)(esp + 16);
    
    f->eax = sys_max_of_four_int(a, b, c, d);
  }
}

void sys_write(int fd, const void *buffer, unsigned size){
  if(fd == 1){
    // STDOUT
    putbuf (buffer, size); 
  }
}

int sys_read(int fd, char *buffer, int size){
  if(fd == 0){
    // STDIN
    int cnt = 0;
    uint8_t c;
    for(int cnt = 0; cnt < size; cnt++) {
      c = input_getc();
      buffer[cnt] = c;
      if(!c) break;
    }
    return size - cnt;
  }
  return 0;
}

int sys_wait(int tid){
  return process_wait (tid); 
}

void sys_exit(int status){
  printf("%s: exit(%d)\n", thread_name(), status);

  struct thread *cur = thread_current ();
  struct child *ch = find_child(cur->tid, &cur->parent->child_list);
  ch->exit_status = status;
  thread_exit();
}

tid_t sys_exec(const char* fname){
  return process_execute (fname); 
}

int sys_fibonacci(int n){
  if(n == 0) return 0;
  else if(n == 1) return 1;
  else if(n == 2) return 1;
  else{
    int arr[3] = {0, 1, 1}, k = 2;
    while(k++ < n){
      arr[0] = arr[1];
      arr[1] = arr[2];
      arr[2] = arr[0] + arr[1];
    }
    return arr[2];
  }
}

int sys_max_of_four_int(int a, int b, int c, int d){
  int ret = a;
  ret = ret > b ? ret : b;
  ret = ret > c ? ret : c;
  ret = ret > d ? ret : d;

  return ret;
}