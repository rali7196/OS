#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include <console.h>

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  return;
  printf ("system call!\n");
  int syscall_num = *((uint32_t*)f->esp);
  if(syscall_num == SYS_WRITE){
	// int* first_arg = f->esp + 1;
	char* second_arg = *((char**)f->esp + 2);
	int third_arg = *((int*)f->esp + 3); 
	putbuf(second_arg, third_arg);
  }
  thread_exit ();
}
