#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "filesys/directory.h"
// clear && make all && pintos -v -k -T 60 --qemu  --disk=tmp.dsk -p tests/filesys/extended/dir-mkdir -a dir-mkdir -p tests/filesys/extended/tar -a tar -- -q  -f run dir-mkdir
// clear && make all && pintos -v -k -T 60  --qemu --disk=tmp.dsk -g fs.tar -a tests/filesys/extended/dir-mkdir.tar -- -q  run 'tar fs.tar /' < /dev/null 2> tests/filesys/extended/dir-mkdir-persistence.errors
/** Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);

/** Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();

  if (format) 
    do_format ();

  free_map_open ();
}

/** Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  free_map_close ();
}

/** Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size, bool is_dir) 
{
  // printf("filesys_create called: %s, is_dir: %d\n", name, is_dir);

  block_sector_t inode_sector = 0;
  // struct dir *dir = dir_open_root ();
  struct dir *dir = path_to_dir (name);
  char* file_name = path_to_name (name);
  // printf("file_name from path_to_name: %s\n", file_name);
  bool success = false;
  if (strcmp(file_name, ".") != 0 && strcmp(file_name, "..") != 0)
    success = (dir != NULL && file_name != NULL
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size, is_dir)
                  && dir_add (dir, file_name, inode_sector, is_dir));
  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  dir_close (dir);
  // free (file_name);
  return success;
}

/** Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
  struct dir *dir = path_to_dir (name);
  struct inode *inode = NULL;

  if (dir != NULL)
    dir_lookup (dir, name, &inode);
  dir_close (dir);

  if (inode == NULL)
    return NULL;

  if (inode_is_dir (inode))
    // return dir_open (inode);
    return (struct file *) dir_open (inode);
  else
    return file_open (inode);
}

/** Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
  struct dir *dir = dir_open_root ();
  bool success = dir != NULL && dir_remove (dir, name);
  dir_close (dir); 

  return success;
}

/** Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}

//assign 4 helper
char*
path_to_name(const char* path_name)
{
  unsigned len = strlen(path_name);
  char copy[len + 1];
  memcpy(copy, path_name, len + 1);

  char *saveptr;
  char *prev = "";
  char *token = strtok_r(copy, "/", &saveptr);
  while(token != NULL){
    prev = token;
    token = strtok_r(NULL, "/", &saveptr);
  }

  len = strlen(prev);
  char *last_dir = (char *) malloc(len + 1);
  memcpy(last_dir, prev, len + 1);
  return last_dir;
}

struct dir*
path_to_dir(const char* path)
{
  unsigned len = strlen(path);
  char copy[len + 1];
  memcpy(copy, path, len + 1);

  struct dir *dir;
  if (copy[0] == '/' || thread_current()->cwd == NULL)
    dir = dir_open_root();
  else
    dir = dir_reopen(thread_current()->cwd);
  
  char *saveptr;
  char *token = strtok_r(copy, "/", &saveptr);
  char *next = strtok_r(NULL, "/", &saveptr);
  while(next){
    if (strcmp(token, ".") != 0){
      struct inode *inode;
      if (strcmp(token, "..") == 0 && !dir_parent_inode(dir))
        return NULL;
      if (!dir_lookup(dir, token, &inode))
        return NULL;

      if (inode_is_dir(inode)){
        dir_close(dir);
        // printf("path_to_dir: %s\n", token);
        dir = dir_open(inode);
      }
      else
        inode_close(inode);
    }
    token = next;
    next = strtok_r(NULL, "/", &saveptr);
  }
  return dir;
}