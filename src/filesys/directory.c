#include "filesys/directory.h"
#include <stdio.h>
#include <string.h>
#include <list.h>
#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "threads/malloc.h"
#include "threads/thread.h"
/** A directory. */
struct dir 
  {
    struct inode *inode;                /**< Backing store. */
    off_t pos;                          /**< Current position. */
    bool deny_write;            /**< Has file_deny_write() been called? */
    bool is_dir;
  };

/** A single directory entry. */
struct dir_entry 
  {
    block_sector_t inode_sector;        /**< Sector number of header. */
    char name[NAME_MAX + 1];            /**< Null terminated file name. */
    bool in_use;                        /**< In use or free? */
    bool is_dir;
  };

/** Creates a directory with space for ENTRY_CNT entries in the
   given SECTOR.  Returns true if successful, false on failure. */
bool
dir_create (block_sector_t sector, size_t entry_cnt)
{
  return inode_create (sector, entry_cnt * sizeof (struct dir_entry), true);
}

/** Opens and returns the directory for the given INODE, of which
   it takes ownership.  Returns a null pointer on failure. */
struct dir *
dir_open (struct inode *inode) 
{
  struct dir *dir = calloc (1, sizeof *dir);
  if (inode != NULL && dir != NULL)
    {
      dir->inode = inode;
      dir->pos = 0;
      return dir;
    }
  else
    {
      inode_close (inode);
      free (dir);
      return NULL; 
    }
}

/** Opens the root directory and returns a directory for it.
   Return true if successful, false on failure. */
struct dir *
dir_open_root (void)
{
  return dir_open (inode_open (ROOT_DIR_SECTOR));
}

/** Opens and returns a new directory for the same inode as DIR.
   Returns a null pointer on failure. */
struct dir *
dir_reopen (struct dir *dir) 
{
  return dir_open (inode_reopen (dir->inode));
}

/** Destroys DIR and frees associated resources. */
void
dir_close (struct dir *dir) 
{
  if (dir != NULL)
    {
      inode_close (dir->inode);
      free (dir);
    }
}

/** Returns the inode encapsulated by DIR. */
struct inode *
dir_get_inode (struct dir *dir) 
{
  return dir->inode;
}

/** Searches DIR for a file with the given NAME.
   If successful, returns true, sets *EP to the directory entry
   if EP is non-null, and sets *OFSP to the byte offset of the
   directory entry if OFSP is non-null.
   otherwise, returns false and ignores EP and OFSP. */
static bool
lookup (const struct dir *dir, const char *name,
        struct dir_entry *ep, off_t *ofsp) 
{
  struct dir_entry e;
  size_t ofs;
  
  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e) 
    if (e.in_use && !strcmp (name, e.name)) 
      {
        if (ep != NULL)
          *ep = e;
        if (ofsp != NULL)
          *ofsp = ofs;
        return true;
      }
  return false;
}

/** Searches DIR for a file with the given NAME
   and returns true if one exists, false otherwise.
   On success, sets *INODE to an inode for the file, otherwise to
   a null pointer.  The caller must close *INODE. */
bool
dir_lookup (const struct dir *dir, const char *name,
            struct inode **inode) 
{
  struct dir_entry e;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  if (lookup (dir, name, &e, NULL))
    *inode = inode_open (e.inode_sector);
  else
    *inode = NULL;

  return *inode != NULL;

}

/** Adds a file named NAME to DIR, which must not already contain a
   file by that name.  The file's inode is in sector
   INODE_SECTOR.
   Returns true if successful, false on failure.
   Fails if NAME is invalid (i.e. too long) or a disk or memory
   error occurs. */
bool
dir_add (struct dir *dir, const char *name, block_sector_t inode_sector, bool is_dir)
{
  // printf("dir_add called: %s, inode_sector: %d, is_dir: %d\n", name, inode_sector, is_dir);
  // printf("dir_add called with inode: %d\n", dir->inode);
  struct dir_entry e;
  off_t ofs;
  bool success = false;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  /* Check NAME for validity. */
  if (*name == '\0' || strlen (name) > NAME_MAX)
    return false;

  /* Check that NAME is not in use. */
  if (lookup (dir, name, NULL, NULL)){
    // printf("dir_add failed: %s\n", name);
    goto done;
  }

  if (!inode_make_parent(inode_get_inumber(dir->inode), inode_sector))
    goto done;
  /* Set OFS to offset of free slot.
     If there are no free slots, then it will be set to the
     current end-of-file.
     
     inode_read_at() will only return a short read at end of file.
     Otherwise, we'd need to verify that we didn't get a short
     read due to something intermittent such as low memory. */
  for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e) 
    if (!e.in_use)
      break;

  /* Write slot. */
  e.in_use = true;
  e.is_dir = is_dir;
  strlcpy (e.name, name, sizeof e.name);
  e.inode_sector = inode_sector;
  // printf("inode_write_at called with inode: %d, name: %s, is_dir: %d, inode_sector: %d\n", dir->inode, e.name, e.is_dir, e.inode_sector);
  success = inode_write_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
  // printf("inode_write_at returned: %d\n", success);
 done:
  // printf("dir_add done: %d\n", success);
  return success;
}

/** Removes any entry for NAME in DIR.
   Returns true if successful, false on failure,
   which occurs only if there is no file with the given NAME. */
bool
dir_remove (struct dir *dir, const char *name) 
{
  struct dir_entry e;
  struct inode *inode = NULL;
  bool success = false;
  off_t ofs;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  /* Find directory entry. */
  if (!lookup (dir, name, &e, &ofs))
    goto done;

  /* Open inode. */
  inode = inode_open (e.inode_sector);
  if (inode == NULL)
    goto done;

  /* If removing a directory, check if it is empty. */
  if (inode_is_dir(inode)) {
    struct dir *subdir = dir_open(inode);
    if (subdir == NULL)
      goto done;

    if (!dir_is_empty(subdir)) {
      dir_close(subdir);
      goto done;
    }

    dir_close(subdir);
  }

  /* Erase directory entry. */
  e.in_use = false;
  if (inode_write_at (dir->inode, &e, sizeof e, ofs) != sizeof e) 
    goto done;

  /* Remove inode. */
  inode_remove (inode);
  success = true;

 done:
  inode_close (inode);
  return success;
}

/** Reads the next directory entry in DIR and stores the name in
   NAME.  Returns true if successful, false if the directory
   contains no more entries. */
bool
dir_readdir (struct dir *dir, char name[NAME_MAX + 1])
{
  struct dir_entry e;

  while (inode_read_at (dir->inode, &e, sizeof e, dir->pos) == sizeof e) 
    {
      dir->pos += sizeof e;
      if (e.in_use)
        {
          strlcpy (name, e.name, NAME_MAX + 1);
          return true;
        } 
    }
  return false;
}

// Assign 4 helper functions

/** Checks if a directory is empty. */
bool
dir_is_empty(struct dir *dir) {
  char name[NAME_MAX + 1];
  return !dir_readdir(dir, name);
}

struct dir* dir_open_path(const char *path) {
  struct dir *dir = NULL;
  struct inode *inode = NULL;

  if (path[0] == '/') {
    dir = dir_open_root();
  } else {
    dir = dir_reopen(thread_current()->cwd);
  }

  char *token, *save_ptr;
  char *path_copy = malloc(strlen(path) + 1);
  strlcpy(path_copy, path, strlen(path) + 1);

  for (token = strtok_r(path_copy, "/", &save_ptr); token != NULL;
       token = strtok_r(NULL, "/", &save_ptr)) {
    if (strcmp(token, ".") == 0) {
      continue;
    } else if (strcmp(token, "..") == 0) {
      inode = dir_parent_inode(dir);
      if (inode == NULL) {
        dir_close(dir);
        free(path_copy);
        return NULL;
      }
      dir_close(dir);
      dir = dir_open(inode);
    } else {
      if (!dir_lookup(dir, token, &inode)) {
        dir_close(dir);
        free(path_copy);
        return NULL;
      }
      dir_close(dir);
      dir = dir_open(inode);
    }
  }

  free(path_copy);
  return dir;
}

struct inode* dir_parent_inode(struct dir* dir) {
  if (dir == NULL) {
    return NULL;
  }

  struct inode *inode = dir_get_inode(dir);
  if (inode == NULL) {
    return NULL;
  }

  block_sector_t parent_sector = inode_get_parent(inode);
  if (parent_sector == ROOT_DIR_SECTOR) {
    return inode_open(parent_sector);
  }

  struct inode *parent_inode = inode_open(parent_sector);
  if (parent_inode == NULL) {
    return NULL;
  }

  return parent_inode;
}
