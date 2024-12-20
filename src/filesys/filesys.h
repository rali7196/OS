#ifndef FILESYS_FILESYS_H
#define FILESYS_FILESYS_H

#include <stdbool.h>
#include "filesys/off_t.h"

/** Sectors of system file inodes. */
#define FREE_MAP_SECTOR 0       /**< Free map file inode sector. */
#define ROOT_DIR_SECTOR 1       /**< Root directory file inode sector. */

/** Block device that contains the file system. */
struct block *fs_device;

struct file_path_info{
    struct dir* dir;
    char* name;
};

void filesys_init (bool format);
void filesys_done (void);
bool filesys_create (const char *name, off_t initial_size, bool is_dir);
struct file *filesys_open (const char *name);
bool filesys_remove (const char *name);

char* path_to_name(const char* path_name);
struct dir* path_to_dir(const char* path_name);
struct dir* parse_path(const char* name);
char* parse_path_name(const char* name);

#endif /**< filesys/filesys.h */
