#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "pagedir.h"
#include "process.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "devices/input.h"
#include "devices/shutdown.h"

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

  if (!validate_user_pointer(f->esp)) { // to-do: add user mem check for read/write
    thread_exit();  // Terminate the process if ESP is invalid
  }

  int syscall_num = *((int*)f->esp);

  if (syscall_num == SYS_HALT){
    shutdown_power_off();
  }
  else if (syscall_num == SYS_EXIT){
    int status = *((int*)f->esp + 1);
    thread_current()->exit_status = status;
    thread_exit();
  }
  else if (syscall_num == SYS_EXEC){  // "pass arguments?"
    char* file_name = *((char**)f->esp + 1);
    if (!validate_user_pointer(file_name)){
      printf("Invalid file name in SYS_EXEC\n"); // debug
      f->eax = -1;
      thread_exit();
    }
    f->eax = process_execute(file_name); // will return -1 if error
  }
  else if (syscall_num == SYS_WAIT){
    tid_t pid = *((tid_t*)f->esp + 1);
    f->eax = process_wait(pid); // process_wait should handle invalid pid
  }
  else if (syscall_num == SYS_CREATE){  // create a new file with the given name and size
    char* file_name = *((char**)f->esp + 1);
    unsigned initial_size = *((unsigned*)f->esp + 2);
    f->eax = filesys_create(file_name, initial_size);
  }
  else if (syscall_num == SYS_REMOVE){  // remove the file with the given name
    char* file_name = *((char**)f->esp + 1);
    f->eax = filesys_remove(file_name);
  }
  else if (syscall_num == SYS_OPEN){
    char* file_name = *((char**)f->esp + 1);
    struct file* file = filesys_open(file_name);
    if (!file){
      f->eax = -1;
    } else {
      for (int i = 2; i < 256; i++){
        if (thread_current()->file_descriptors_table[i] == NULL){
          thread_current()->file_descriptors_table[i] = file;
          f->eax = i;
          break;
        }
      }
    }
  }
  else if (syscall_num == SYS_FILESIZE){  // return the size of the file with the given fd
    int fd = *((int*)f->esp + 1);
    f->eax = file_length(thread_current()->file_descriptors_table[fd]);
  }
  else if (syscall_num == SYS_READ){  
    int fd = *((int*)f->esp + 1);
    void* buffer = *((void**)f->esp + 2);
    unsigned size = *((unsigned*)f->esp + 3);
    if (fd == 0){ // read from input_getc
      for (unsigned i = 0; i < size; i++){
        *((char*)buffer + i) = input_getc();
      }
      f->eax = size;
    } else {  // read from the file with the given fd
      f->eax = file_read(thread_current()->file_descriptors_table[fd], buffer, size);
    }
  }
  else if (syscall_num == SYS_WRITE){
    int fd = *((int*)f->esp + 1);
    void* buffer = *((void**)f->esp + 2);
    unsigned size = *((unsigned*)f->esp + 3);
    if (fd == 1){  // write to the console
      putbuf(buffer, size);
      f->eax = size;
    } else {  // write to the file with the given fd
      f->eax = file_write(thread_current()->file_descriptors_table[fd], buffer, size);
    }
  }
  else if (syscall_num == SYS_SEEK){
    int fd = *((int*)f->esp + 1);
    unsigned position = *((unsigned*)f->esp + 2);
    file_seek(thread_current()->file_descriptors_table[fd], position);
  }
  else if (syscall_num == SYS_TELL){
    int fd = *((int*)f->esp + 1);
    f->eax = file_tell(thread_current()->file_descriptors_table[fd]);
  }
  else if (syscall_num == SYS_CLOSE){
    int fd = *((int*)f->esp + 1);
    file_close(thread_current()->file_descriptors_table[fd]);
    thread_current()->file_descriptors_table[fd] = NULL;
  }
  else {  // to-do: add proper error handling
    printf("Invalid system call number\n");
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