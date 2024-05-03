#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "pagedir.h"

static void syscall_handler (struct intr_frame *);
static bool validate_user_pointer (const void *ptr);  // implemented for project 2

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");

  if (!validate_user_pointer(f->esp)) {
      thread_exit();  // Terminate the process if ESP is invalid
  }

  int syscall_number = *(int *)(f->esp);  // get the system call number

  switch (syscall_number) // added system call numbers for project 2
  {
    case SYS_READ:
      
      break;
    case SYS_WRITE:
      /* code */
      break;
    default:
      printf("Invalid system call number\n");
      break;
  }
  thread_exit ();
}

/*Return false if the given pointer is NULL, out of user space, or not in its page*/
static bool 
validate_user_pointer (const void *ptr) {
  if (!ptr)
    return false;
  if (!is_user_vaddr(ptr))
    return false;
  if (!pagedir_get_page(thread_current()->pagedir, ptr))
    return false;
  return true;
}