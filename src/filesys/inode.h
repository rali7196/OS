#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <stdbool.h>
#include "filesys/off_t.h"
#include "devices/block.h"
#include <list.h>

#define DIRECT_SIZE 120
#define INDIRECT_SIZE 128
struct bitmap;
struct inode_disk
  {
    // block_sector_t start;               /**< First data sector. */
    // off_t length;                       /**< File size in bytes. */
    // unsigned magic;                     /**< Magic number. */
    // uint32_t unused[125];               /**< Not used. */
    off_t length;                       /**< File size in bytes. */
    unsigned magic;                     /**< Magic number. */
    block_sector_t direct[DIRECT_SIZE];
    block_sector_t indirect;
    block_sector_t double_indirect;
    // unsigned counter;
    unsigned max_size;
    block_sector_t location;
    bool is_dir;
    block_sector_t parent;
  
  };
struct inode 
  {
    struct list_elem elem;              /**< Element in inode list. */
    block_sector_t sector;              /**< Sector number of disk location. */
    int open_cnt;                       /**< Number of openers. */
    bool removed;                       /**< True if deleted, false otherwise. */
    int deny_write_cnt;                 /**< 0: writes ok, >0: deny writes. */
    struct inode_disk data;             /**< Inode content. */

    bool is_dir;                        /**< True if inode is a directory. */
    block_sector_t parent;              /**< Parent directory's sector number. */
    bool expanded;
  };

void inode_init (void);
bool inode_create (block_sector_t, off_t, bool is_dir);
struct inode *inode_open (block_sector_t);
struct inode *inode_reopen (struct inode *);
block_sector_t inode_get_inumber (const struct inode *);
void inode_close (struct inode *);
void inode_remove (struct inode *);
off_t inode_read_at (struct inode *, void *, off_t size, off_t offset);
off_t inode_write_at (struct inode *, const void *, off_t size, off_t offset);
void inode_deny_write (struct inode *);
void inode_allow_write (struct inode *);
off_t inode_length (const struct inode *);

bool inode_is_dir (const struct inode *inode);
block_sector_t inode_get_parent (const struct inode *inode);
bool inode_make_parent(block_sector_t parent_sector, block_sector_t child_sector);

#endif /**< filesys/inode.h */
