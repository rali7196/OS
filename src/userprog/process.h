#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

struct exec_args {
  char** parsed_argv;
  int parsed_argc;
  int missing_file_status;
};

struct vm_entry {
  //add all fields outlined in 
};

tid_t process_execute (const char *file_name, struct exec_args* local_args);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);



#endif /**< userprog/process.h */
