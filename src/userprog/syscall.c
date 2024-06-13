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
#include "threads/synch.h"
#include <string.h>
#include "threads/malloc.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "filesys/free-map.h"
#include "filesys/file.h"



static void syscall_handler (struct intr_frame *);
static bool validate_user_pointer (const void *ptr);  // implemented for project 2
static bool validate_file_descriptor(int fd);
// static struct lock fs_lock;  // lock for file system operations


//clear && make all && pintos -v -k -T 60 --qemu --gdb --filesys-size=2 -p tests/userprog/sc-bad-sp -a sc-bad-sp -- -q  -f run sc-bad-sp
//clear && make all && pintos -v -k -T 60 --qemu --gdb --filesys-size=2 -p tests/userprog/args-none -a args-none -- -q  -f run args-none
//clear && make all && pintos -v -k -T 60 --qemu --gdb --filesys-size=2 -p tests/userprog/sc-bad-arg -a sc-bad-arg -- -q  -f run sc-bad-arg
//clear && make all && pintos -v -k -T 60 --qemu --gdb --filesys-size=2 -p tests/userprog/sc-boundary-3 -a sc-boundary-3 -- -q  -f run sc-boundary-3
//clear && make all && pintos -v -k -T 60 --qemu --gdb --filesys-size=2 -p tests/userprog/create-null -a create-null -- -q  -f run create-null
//clear && make all && pintos -v -k -T 60 --qemu --gdb --filesys-size=2 -p tests/userprog/open-null -a open-null -- -q  -f run open-null
//clear && make all && pintos -v -k -T 60 --qemu --gdb --filesys-size=2 -p tests/userprog/bad-read -a bad-read -- -q  -f run bad-read 
//clear && make all && pintos -v -k -T 60 --qemu --filesys-size=2 -p tests/userprog/bad-read -a bad-read -- -q  -f run bad-read < /dev/null
//clear && make all && pintos -v -k -T 60 --qemu --gdb --filesys-size=2 -p tests/userprog/exec-once -a exec-once -p tests/userprog/child-simple -a child-simple -- -q  -f run exec-once
//clear && make all && pintos -v -k -T 60 --qemu --gdb --filesys-size=2 -p tests/userprog/wait-simple -a wait-simple -p tests/userprog/child-simple -a child-simple -- -q  -f run wait-simple
//clear && make all && pintos -v -k -T 60 --qemu --gdb --filesys-size=2 -p tests/userprog/args-multiple -a args-multiple -- -q  -f run 'args-multiple some arguments for you!'
//clear && make all && pintos -v -k -T 60 --qemu --gdb --filesys-size=2 -p tests/userprog/exec-arg -a exec-arg -p tests/userprog/child-args -a child-args -- -q  -f run exec-arg
//clear && make all && pintos -v -k -T 60 --qemu --gdb --filesys-size=2 -p tests/filesys/base/syn-remove -a syn-remove -- -q  -f run syn-remove

//clear && make all && pintos -v -k -T 60 --qemu --gdb --filesys-size=2 -p tests/userprog/exec-missing -a exec-missing -- -q  -f run exec-missing


//clear && make all && pintos -v -k -T 60 --qemu --gdb --filesys-size=2 -p tests/userprog/exec-bound-2 -a exec-bound-2 -- -q  -f run exec-bound-2
void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&fs_lock);
} 
// 0xbfffff18
static void
syscall_handler (struct intr_frame *f) 
{

  if(thread_current()->cwd2 == NULL){
    thread_current()->cwd2 = malloc(256);
    strlcpy(thread_current()->cwd2, "/", 2);
  }
  //check lower bound of memory to ensure that it is okay
  if (!validate_user_pointer(f->esp)) {
    // printf("Invalid ESP in syscall_handler\n"); // debug
    thread_current()->exit_status = -1;
    thread_exit();  // Terminate the process if ESP is invalid
  }

  //check upper bound of memory to ensure that it is okay
  if (!validate_user_pointer(f->esp+sizeof(int))) {
    // printf("Invalid ESP in syscall_handler\n"); // debug
    thread_current()->exit_status = -1;
    thread_exit();  // Terminate the process if ESP is invalid
  }

  int syscall_num = *((int*)f->esp);

  if (syscall_num == SYS_HALT){
    shutdown_power_off();
  }
  else if (syscall_num == SYS_EXIT){
    if (!validate_user_pointer(f->esp+sizeof(int))) {
      // printf("Invalid ESP in syscall_handler\n"); // debug
      thread_current()->exit_status = -1;
      thread_exit();  // Terminate the process if ESP is invalid
    }
    int status = *((int*)f->esp + 1);

    // close all files/dirs
    for (int fd = 2; fd < MAX_FILE_DESCRIPTORS; fd++){
      if (thread_current()->file_descriptors_table[fd] != NULL)
      {
        struct inode* inode = file_get_inode(thread_current()->file_descriptors_table[fd]);
        if(inode == NULL){
          continue;
        }
        // if(inode_is_dir(inode)){
        //   dir_close(thread_current()->file_descriptors_table[fd]);
        // }
          
        else{
          file_close (thread_current()->file_descriptors_table[fd]);
        }
        thread_current()->file_descriptors_table[fd] = NULL;
      }
    }
    
    // close cwd
    if (thread_current()->cwd){
      dir_close(thread_current()->cwd);
    }

    // printf("%s: exit(%d)\n", thread_current()->name, status);
    thread_current()->exit_status = status;
    thread_exit();
  }

  else if (syscall_num == SYS_EXEC){  // "pass arguments?"

    lock_acquire(&fs_lock);


    //clear && make all && pintos -v -k -T 60 --qemu --gdb --filesys-size=2 -p tests/userprog/exec-bound-3 -a exec-bound-3 -- -q  -f run exec-bound-3


    if (!validate_user_pointer((char **)f->esp + 1)) {
        f->eax = -1;
        lock_release(&fs_lock);

        thread_current()->exit_status = -1;
        thread_exit();
    }

    if (!validate_user_pointer((char **)f->esp + 2)) {
        f->eax = -1;
        lock_release(&fs_lock);

        thread_current()->exit_status = -1;
        thread_exit();
    }

    char* file_name = *((char**)f->esp + 1);

    char curr = 'a';

    for(int i = 0; curr != '\0'; i++){

      void *curr_ptr = (void*)(file_name)+i;
      if (!validate_user_pointer(curr_ptr)) {
        f->eax = -1;
        lock_release(&fs_lock);
        thread_current()->exit_status = -1;
        thread_exit();
      }

      curr = *(char*)curr_ptr;
    }
    



    // Validate the file_name pointer
    if (file_name == NULL || !validate_user_pointer(file_name) || strlen(file_name) == 0) {
        f->eax = -1;
        lock_release(&fs_lock);
        thread_current()->exit_status = -1;
        thread_exit(); 
    }
    // to-do: Check if the file exists 

    // struct file* file_status = filesys_open(file_name);
    // printf("opening file: %s\n", file_name);

    // if(!file_status){
    //   lock_release(&fs_lock);

    //   f->eax = -1;
    //   // thread_current()->exit_status = -1;
    //   // thread_exit();
    //   printf("Terminating thread due to bad file name\n");
    //   return;
    // }

    // file_close(file_status);

    struct exec_args *local_args = malloc(sizeof(struct exec_args));
    local_args->missing_file_status = 0;


    tid_t pid = process_execute(file_name, local_args); // will return -1 if error
    struct file* file_status = filesys_open(local_args->parsed_argv[local_args->parsed_argc-1]);
    if(!file_status){
      f->eax = -1;
      file_close(file_status);
      lock_release(&fs_lock);//
      return;
    }
    file_close(file_status);


    if(local_args->missing_file_status){
      f->eax = -1;
      lock_release(&fs_lock);
      return;
    }
    if (pid == TID_ERROR || pid == -1) {
      // printf("Error in process_execute\n");
      f->eax = -1;
      lock_release(&fs_lock);
      
      thread_current()->exit_status = -1;
      thread_exit();
    }
    f->eax = pid;
    // free(local_args);
    lock_release(&fs_lock);

  }
  
  else if (syscall_num == SYS_WAIT){
    if (!is_user_vaddr(f->esp) || !is_user_vaddr((tid_t *)f->esp + 1)) {
      thread_current()->exit_status = -1;
      thread_exit();  // Terminate the process if ESP is invalid
    }
    tid_t pid = *((tid_t*)f->esp + 1);
    f->eax = process_wait(pid); // process_wait should handle other invalid pid
  }

  else if (syscall_num == SYS_CREATE){  // create a new file with the given name and size

    //validate first argument

    char* file_name = *((char**)f->esp + 1);
    if (!validate_user_pointer(file_name)) {
    // printf("Invalid ESP in syscall_handler\n"); // debug
      thread_current()->exit_status = -1;
      thread_exit();  // Terminate the process if ESP is invalid
    }
    if (!validate_user_pointer(file_name + strlen(file_name))) {
    // printf("Invalid ESP in syscall_handler\n"); // debug
      thread_current()->exit_status = -1;
      thread_exit();  // Terminate the process if ESP is invalid
    }
    unsigned initial_size = *((unsigned*)f->esp + 2);
    //validate second argument
    if (!validate_user_pointer(f->esp+2)) {
    // printf("Invalid ESP in syscall_handler\n"); // debug
      thread_current()->exit_status = -1;
      thread_exit();  // Terminate the process if ESP is invalid
    }
    lock_acquire(&fs_lock);
    f->eax = filesys_create(file_name, initial_size, false);
    lock_release(&fs_lock);
  }
  else if (syscall_num == SYS_REMOVE){  // remove the file with the given name
    char* file_name = *((char**)f->esp + 1);
    lock_acquire(&fs_lock);
    f->eax = filesys_remove(file_name);
    lock_release(&fs_lock);
  }
  else if (syscall_num == SYS_OPEN){
    char* file_name = *((char**)f->esp + 1);
    if (!validate_user_pointer(file_name)) {
    // printf("Invalid ESP in syscall_handler\n"); // debug
      thread_current()->exit_status = -1;
      thread_exit();  // Terminate the process if ESP is invalid
    }
    lock_acquire(&fs_lock);

    //have full file path, just need to parse it and then determine if its a directory or a file
    //only have to do string parsing if im not in the root directory and my file path is long
    if(strlen(file_name) == 0){
      f->eax = -1;
      lock_release(&fs_lock);
      return;
    }
    char* temp = malloc(512);
    if(strcmp(thread_current()->cwd2, "/") != 0 || strlen(file_name) > NAME_MAX){
      //concatenate current path to cwd
      strlcpy(temp, thread_current()->cwd2, strlen(thread_current()->cwd2)+1);
      strlcat(temp, file_name, strlen(file_name) + strlen(temp)+1);
      strlcat(temp, "/", strlen(temp)+strlen("/") + 1);

      // struct dir* my_dir = parse_path()
    } else {
      strlcpy(temp, file_name, strlen(file_name)+1);
    }


    struct file* file = filesys_open(temp);
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
    lock_release(&fs_lock);
  }
  else if (syscall_num == SYS_FILESIZE){  // return the size of the file with the given fd
    int fd = *((int*)f->esp + 1);
    if (!validate_file_descriptor(fd)){
      f->eax = -1;
      thread_current()->exit_status = -1;
      thread_exit();
    }
    lock_acquire(&fs_lock);
    f->eax = file_length(thread_current()->file_descriptors_table[fd]);
    lock_release(&fs_lock);
  }
  else if (syscall_num == SYS_READ){  
    if (!validate_user_pointer(f->esp+1)) {
      thread_current()->exit_status = -1;
      // printf("Invalid fd in SYS_READ\n"); // debug
      thread_exit();
    }
    int fd = *((int*)f->esp + 1);

    if (!validate_user_pointer(f->esp + 2)) {
      thread_current()->exit_status = -1;
      // printf("Invalid buffer in SYS_READ\n"); // debug
      thread_exit();
    }

    void* buffer = *((void**)f->esp + 2);
    if (!validate_user_pointer(f->esp + 3)) {
      thread_current()->exit_status = -1;
      // printf("Invalid buffer size in SYS_READ\n"); // debug
      thread_exit();
    }    
    unsigned size = *((unsigned*)f->esp + 3);


    if (!validate_user_pointer(buffer) || !validate_user_pointer(buffer + size - 1)){ // check buffer validity
      f->eax = -1;
      thread_current()->exit_status = -1;
      // printf("Invalid end of buffer in SYS_READ\n"); // debug
      thread_exit();
    }
    lock_acquire(&fs_lock);
    if (fd == 0){ // read from input_getc
      for (unsigned i = 0; i < size; i++){
        *((char*)buffer + i) = input_getc();
      }
      f->eax = size;
    } else {  // read from the file with the given fd
      if (!validate_file_descriptor(fd)){
        f->eax = -1;
        thread_current()->exit_status = -1;
        thread_exit();
        lock_release(&fs_lock);
      }else{
        f->eax = file_read(thread_current()->file_descriptors_table[fd], buffer, size);
      }
    }
    lock_release(&fs_lock);
  }
  else if (syscall_num == SYS_WRITE){
    // printf("calling sys write");
    int fd = *((int*)f->esp + 1);
    void* buffer = *((void**)f->esp + 2);
    unsigned size = *((unsigned*)f->esp + 3);
    if (!validate_user_pointer(buffer) || !validate_user_pointer(buffer + size - 1)){ // check buffer validity
      // printf("Invalid buffer in SYS_WRITE\n"); // debug
      f->eax = -1;
      thread_current()->exit_status = -1;
      thread_exit();
    }
    if (fd == 1){  // write to the console
      putbuf(buffer, size);
      f->eax = size;
    } else {  // write to the file with the given fd
      if (!validate_file_descriptor(fd)){
        f->eax = -1;
        thread_current()->exit_status = -1;
        thread_exit();
      }
      lock_acquire(&fs_lock);
      struct file* to_write = thread_current()->file_descriptors_table[fd];
      if(to_write->is_dir){
        f->eax = -1;
        lock_release(&fs_lock);
        return;
      }
      f->eax = file_write(thread_current()->file_descriptors_table[fd], buffer, size);
      lock_release(&fs_lock);
    }
  }
  else if (syscall_num == SYS_SEEK){
    int fd = *((int*)f->esp + 1);
    if (!validate_file_descriptor(fd)){
      f->eax = -1;
      thread_current()->exit_status = -1;
      thread_exit();
    }
    if (!validate_user_pointer((void*)(f->esp + 2))) {
      // printf("Invalid position in SYS_SEEK\n"); // debug
      thread_current()->exit_status = -1;
      thread_exit();
    }
    unsigned position = *((unsigned*)f->esp + 2);
    lock_acquire(&fs_lock);
    file_seek(thread_current()->file_descriptors_table[fd], position);
    lock_release(&fs_lock);
  }
  else if (syscall_num == SYS_TELL){
    int fd = *((int*)f->esp + 1);
    if (!validate_file_descriptor(fd)){
      f->eax = -1;
      thread_current()->exit_status = -1;
      thread_exit();
    }
    lock_acquire(&fs_lock);
    f->eax = file_tell(thread_current()->file_descriptors_table[fd]);
    lock_release(&fs_lock);
  }
  else if (syscall_num == SYS_CLOSE){
    int fd = *((int*)f->esp + 1);
    if (!validate_file_descriptor(fd)){
      f->eax = -1;
      thread_current()->exit_status = -1;
      thread_exit();
    }
    lock_acquire(&fs_lock);

    struct inode* inode = file_get_inode(thread_current()->file_descriptors_table[fd]);
    if (inode == NULL){
      f->eax = -1;
      lock_release(&fs_lock);
      thread_current()->exit_status = -1;
      thread_exit();
    }

    // if (inode_is_dir(inode)){
    //   dir_close(thread_current()->file_descriptors_table[fd]);
    // }
    else{
      file_close(thread_current()->file_descriptors_table[fd]);
    }
    thread_current()->file_descriptors_table[fd] = NULL;
    lock_release(&fs_lock);
  }
  else if (syscall_num == SYS_MKDIR){
    lock_acquire(&fs_lock);

    if (!validate_user_pointer((void*)(f->esp + 1))) {
      f->eax = -1;
      lock_release(&fs_lock);
      thread_current()->exit_status = -1;
      thread_exit();
    }

    char* path = (char*)*((int *)f->esp + 1);
    if(strlen(path) == 0){
      f->eax = false;
      lock_release(&fs_lock);
      return;
    }
    //need to parse the string, and then keep calling dir look up to find the inodes
    block_sector_t new_dir_sector = 0;
    free_map_allocate(1, &new_dir_sector);
    
    // f->eax = filesys_create(path, 0, true);
    f->eax = dir_create(new_dir_sector, 1);
    struct dir* to_add;

    to_add = parse_path(path);
    
    dir_add(to_add, path, new_dir_sector, true);

    lock_release(&fs_lock);
  }
  else if (syscall_num == SYS_READDIR){
    lock_acquire(&fs_lock);
    if (!validate_user_pointer((void*)(f->esp + 2))){
      f->eax = -1;
      lock_release(&fs_lock);
      thread_current()->exit_status = -1;
      thread_exit();
    }

    char* path = (char*)*((int *)f->esp + 2);
    int fd = *((int*)f->esp + 1);
    if (!validate_file_descriptor(fd)){
      f->eax = -1;
      thread_current()->exit_status = -1;
      thread_exit();
    }
    struct file* file = thread_current()->file_descriptors_table[fd];
    if (file == NULL){
      f->eax = false;
    }

    struct inode* inode = file_get_inode(file);
    if(inode == NULL){
      f->eax = false;
    }
    if(!inode_is_dir(inode)){
      f->eax = false;
    }

    struct dir* dir = (struct dir*) file;
    if(!dir_readdir(dir, path)) {
      f->eax = false;
    }

    f->eax = true;

    lock_release(&fs_lock);
  }
  else if (syscall_num == SYS_ISDIR){

    if ((!validate_user_pointer((void*)(f->esp + 1)))){
      f->eax = -1;
      thread_current()->exit_status = -1;
      thread_exit();
    }

    int fd = *((int*)f->esp + 1);
    if (!validate_file_descriptor(fd)){
      f->eax = -1;
      thread_current()->exit_status = -1;
      thread_exit();
    }

    struct file* file = thread_current()->file_descriptors_table[fd];
    if (file == NULL) {
      f->eax = false;
    }

    struct inode* inode = file_get_inode(file);
    if(inode == NULL){
      f->eax = false;
    }
    if(!inode_is_dir(inode)){
      f->eax = false;
    }

    f->eax = true;
  }
  else if (syscall_num == SYS_INUMBER){
    int fd = *((int*)f->esp + 1);
    if (!validate_file_descriptor(fd)){
      f->eax = -1;
      thread_current()->exit_status = -1;
      thread_exit();
    }
    struct file* file = thread_current()->file_descriptors_table[fd];
    if (file == NULL){
      f->eax = -1;
    }

    struct inode* inode = file_get_inode(file);
    if(inode == NULL){
      f->eax = -1;
    }

    block_sector_t inumber = inode_get_inumber(inode);
    f->eax = inumber;
  }
  else if (syscall_num == SYS_CHDIR){
    if (!validate_user_pointer((void*)(f->esp + 1))) {
      f->eax = -1;
      lock_release(&fs_lock);
      thread_current()->exit_status = -1;
      thread_exit();
    }

    char* path = (char*)*((int *)f->esp + 1);
    char* temp = malloc(512);

    strlcpy(temp, path, strlen(path)+1);
    strlcat(temp, "/", strlen(temp)+strlen("/") + 1);
    strlcat(thread_current()->cwd2, temp, strlen(thread_current()->cwd2) + strlen(temp)+1);
    // strlcpy(thread_current()->cwd2, path, strlen(path)+1);
    
    return;
    

  }

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

static bool
validate_file_descriptor(int fd){
  if (fd < 2 || fd > 255 || !thread_current()->file_descriptors_table[fd]){
    // printf("Invalid file descriptor in validate_file_discriptor\n"); // debug
    return false;
  }
  return true;
}

