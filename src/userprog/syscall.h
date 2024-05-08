#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

struct lock fs_lock;  // lock for file system operations

void syscall_init (void);

#endif /**< userprog/syscall.h */
