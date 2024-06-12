#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include <stdio.h>


/** Identifies an inode. */
#define INODE_MAGIC 0x494e4f44
#define PTR_SIZE 4

#define DIRECT_SIZE 120
#define INDIRECT_SIZE 128

/** On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */



struct block_table {
  block_sector_t blocks[INDIRECT_SIZE];
};


//modify this to store block sector ts, then on expansion allocate a new block_table struct
struct double_block_table {
  // struct block_table blocks[128];
  block_sector_t blocks[INDIRECT_SIZE];
};

//need to use block_sector_t to maintain persistence.
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

/** Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
 bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

/** In-memory inode. */
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

/** Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static block_sector_t
byte_to_sector (const struct inode_disk *disk, off_t pos) 
{
  ASSERT (disk != NULL);
  // struct inode_disk disk = inode->data;
  unsigned block_number = (pos / BLOCK_SECTOR_SIZE);
  unsigned first_max = DIRECT_SIZE;
  unsigned second_max = DIRECT_SIZE + INDIRECT_SIZE;

  //calculate the amount of space you need to allocate

  if(block_number < first_max){
    block_sector_t res = disk->direct[block_number];
    return res;
  } 
  
  else if (block_number <= second_max){
    //read block containing pointers into buffer
    void* indirect_buffer = malloc(sizeof(struct block_table));
    block_read(fs_device, disk->indirect, indirect_buffer);
    block_sector_t res = ((struct block_table*) indirect_buffer)->blocks[block_number - first_max];
    free(indirect_buffer);
    return res;
  } 

  else {
    int indirect_index = (block_number - second_max) / (BLOCK_SECTOR_SIZE / PTR_SIZE);
    int direct_index = (block_number - second_max) % (BLOCK_SECTOR_SIZE / PTR_SIZE);

    void* indirect_buffer = malloc(sizeof(struct double_block_table));
    block_read(fs_device, disk->double_indirect, indirect_buffer);
    struct double_block_table* double_indirect_table = (struct double_block_table*) indirect_buffer;
    block_sector_t indirect_block_table_sector = double_indirect_table->blocks[indirect_index];

    
    void* direct_buffer = malloc(sizeof(struct block_table));
    block_read(fs_device, indirect_block_table_sector, direct_buffer);
    struct block_table* indirect_block_table = (struct block_table*) direct_buffer;
    block_sector_t res = indirect_block_table->blocks[direct_index];
    
    // block_sector_t indirect_block = ( (struct double_block_table*) indirect_buffer)->blocks[indirect_index];

    free(indirect_buffer);
    free(direct_buffer);
    return res; 
  }


  // if (pos < inode->data.length)
  //   return inode->data.start + pos / BLOCK_SECTOR_SIZE;
  // else
  //   return -1;
}


static bool allocate_new_sector(struct inode_disk *disk, off_t pos){
  ASSERT (disk != NULL);
  // struct inode_disk disk = inode->data;
  unsigned block_number = (pos / BLOCK_SECTOR_SIZE);
  unsigned first_max = DIRECT_SIZE;
  unsigned second_max = DIRECT_SIZE + INDIRECT_SIZE;

  //calculate the amount of space you need to allocate

  if(block_number < first_max){
    if(disk->direct[block_number] == 0){
      free_map_allocate(1, &(disk->direct[block_number]));
    }
    block_sector_t res = disk->direct[block_number];
    block_write(fs_device, disk->location, disk);
    return res;
  } 
  
  else if (block_number <= second_max){
    //read block containing pointers into buffer
    void* indirect_buffer = malloc(sizeof(struct block_table));
    block_read(fs_device, disk->indirect, indirect_buffer);
    struct block_table* indirect_table = indirect_buffer;
    if(indirect_table->blocks[block_number-first_max] == 0){
      free_map_allocate(1, &indirect_table->blocks[block_number-first_max]);
    }


    block_sector_t res = ((struct block_table*) indirect_buffer)->blocks[block_number - first_max];
    block_write(fs_device, disk->indirect, indirect_table);
    block_write(fs_device, disk->location, disk);

    free(indirect_buffer);
    return res;
  } 

  else {
    int indirect_index = (block_number - second_max) / (BLOCK_SECTOR_SIZE / PTR_SIZE);
    int direct_index = (block_number - second_max) % (BLOCK_SECTOR_SIZE / PTR_SIZE);

    //getting the double indirect block table and proper single indirect table
    void* indirect_buffer = malloc(sizeof(struct double_block_table));
    block_read(fs_device, disk->double_indirect, indirect_buffer);
    struct double_block_table* double_indirect_table = (struct double_block_table*) indirect_buffer;
    if(double_indirect_table->blocks[indirect_index] == 0){
      free_map_allocate(1, &(double_indirect_table->blocks[indirect_index]));
    }
    //getting 
    block_sector_t indirect_block_table_sector = double_indirect_table->blocks[indirect_index];

    //getting the indirect table and proper sector
    void* direct_buffer = malloc(sizeof(struct block_table));
    block_read(fs_device, indirect_block_table_sector, direct_buffer);
    struct block_table* indirect_block_table = (struct block_table*) direct_buffer;
    if(indirect_block_table->blocks[direct_index] == 0){
      free_map_allocate(1, &(indirect_block_table->blocks[direct_index]));
    }
    block_sector_t res = indirect_block_table->blocks[direct_index];
    block_write(fs_device, res, indirect_block_table);
    block_write(fs_device, indirect_block_table_sector, double_indirect_table);
    block_write(fs_device, disk->location, disk);
    
    // block_sector_t indirect_block = ( (struct double_block_table*) indirect_buffer)->blocks[indirect_index];

    free(indirect_buffer);
    free(direct_buffer);
    return res; 
  } 
}

//modify this to return the location of the next available block

/** List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/** Initializes the inode module. */
void
inode_init (void) 
{
  list_init (&open_inodes);
}

/** Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length, bool is_dir)
{
  // printf("inode_create called: sector: %d, length: %d, is_dir: %d\n", sector, length, is_dir);
  struct inode_disk *disk_inode = NULL;
  bool success = false;

  ASSERT (length >= 0);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

  disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL){
    size_t sectors = bytes_to_sectors (length);
    //initialize the blocks here
    disk_inode->length = length;
    disk_inode->magic = INODE_MAGIC;
    disk_inode->location = sector;
    disk_inode->is_dir = is_dir;
    disk_inode->parent = ROOT_DIR_SECTOR;

    static char zeros[BLOCK_SECTOR_SIZE];
    //allocating initial sectors

    //allocating indirect and doubly indrirect, allocate first direct block, then add from there
    // free_map_allocate(1, &disk_inode->direct[0]);
    // disk_inode->counter += 1;
    // block_sector_t first = disk_inode->direct[0];

    free_map_allocate(1, &disk_inode->indirect);
    free_map_allocate(1, &disk_inode->double_indirect);
    //need to write relevant structs into these blocks

    struct block_table indirect;
    struct double_block_table double_indirect;
    for(int i = 0; i < INDIRECT_SIZE; i++){
      indirect.blocks[i] = 0;
    }
    for(int i = 0; i < INDIRECT_SIZE; i++){
      double_indirect.blocks[i] = 0;
    }
    block_write(fs_device, disk_inode->indirect, &indirect);
    block_write(fs_device, disk_inode->double_indirect, &double_indirect);
    // printf("size of indirect, double_indirect: %li, %li\n", sizeof(indirect), sizeof(double_indirect));

    //allocate proper amount of sectors

    for(size_t i = 0; i < sectors ; i++){
      allocate_new_sector(disk_inode, i * BLOCK_SECTOR_SIZE);
      // free_map_allocate(1, next);
    }


    block_write (fs_device, sector, disk_inode);

    //zeroing out the file
    if (sectors > 0) {
      size_t i;
      
      for (i = 0; i < sectors; i++){
        block_sector_t next = byte_to_sector(disk_inode, i * BLOCK_SECTOR_SIZE);
        block_write (fs_device, next, zeros);
      }
    }
    success = true; 

      
      // if (free_map_allocate (sectors, &disk_inode->start)) 
      //   {
      //     block_write (fs_device, sector, disk_inode);
      //     if (sectors > 0) 
      //       {
      //         static char zeros[BLOCK_SECTOR_SIZE];
      //         size_t i;
              
      //         for (i = 0; i < sectors; i++) 
      //           block_write (fs_device, disk_inode->start + i, zeros);
      //       }
      //     success = true; 
      //   } 
      free (disk_inode);
    }
  return success;//
}


/** Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e)) 
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector) 
        {
          inode_reopen (inode);
          return inode; 
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  block_read (fs_device, inode->sector, &inode->data);//ask what this does??
  return inode;
}

/** Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/** Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/** Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode) 
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);
 
      /* Deallocate blocks if removed. */
      if (inode->removed) 
        {
          free_map_release (inode->sector, 1);

          size_t total_sectors = bytes_to_sectors(inode->data.length);
          for(size_t i = 0; i < total_sectors; i++){
            block_sector_t next = byte_to_sector(&inode->data, i * BLOCK_SECTOR_SIZE);
            free_map_release(next, 1);
          }
          // free_map_release (byte_to_sector(),
          //                   bytes_to_sectors (inode->data.length)); 
        }

      free (inode); 
    }
}

/** Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode) 
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/** Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */

   //modify this to read from block tables (focus on getting the right blocks to read)
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset) 
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  uint8_t *bounce = NULL;

  while (size > 0) 
    {
      /* Disk sector to read, starting byte offset within sector. */
      //modify this to find proper sector to read 
      block_sector_t sector_idx = byte_to_sector (&inode->data, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Read full sector directly into caller's buffer. */
          block_read (fs_device, sector_idx, buffer + bytes_read);
        }
      else 
        {
          /* Read sector into bounce buffer, then partially copy
             into caller's buffer. */
          if (bounce == NULL) 
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }
          block_read (fs_device, sector_idx, bounce);
          memcpy (buffer + bytes_read, bounce + sector_ofs, chunk_size);
        }
      
      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
  free (bounce);

  return bytes_read;
}


/** Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */

   //modify this to write to block tables (focus on getting the right blocks to write to )
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset) 
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  uint8_t *bounce = NULL;
  // print("sizeof")

  if (inode->deny_write_cnt)
    return 0;



  //need to allocate the proper amount of sectors at write time

  // off_t offset_2 = offset;



  off_t offset_ = offset;
  // bool expanded = false;

  // off_t size2 = size;

  // if(inode->data.length == 239){
  //   printf("hello");
  // }
  int bytes_expanded = 0;


  static char zeros[BLOCK_SECTOR_SIZE];
  //if past end of file
  if(offset > inode->data.length){
    for(unsigned i = 0; i < bytes_to_sectors(offset); i++){
      // if(i == 135){
      //   printf("hello");
      // }
      allocate_new_sector(&(inode->data), i * BLOCK_SECTOR_SIZE);
      block_sector_t sector_idx = byte_to_sector(&inode->data, i * BLOCK_SECTOR_SIZE);
      if(i + 1 == bytes_to_sectors(offset)){
        // void* indirect = malloc(BLOCK_SECTOR_SIZE);
        // block_read(fs_device, sector_idx, indirect);
        // printf("almost there");
        inode->data.length += (offset - (i*512));
        // free(indirect);
      } else {
        inode->data.length += 512;  
      }
      block_write(fs_device, sector_idx, zeros);

      block_write(fs_device, inode->data.location, &(inode->data));      
    }
  }


//need to create some way to check if I need more space
  while (size > 0) 
    {
      /* Sector to write, starting byte offset within sector. */
      //calculate index of sector to write to
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      //cant write more bytes than are left in sector

      // off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      //if inode left is less than sector left, we can expand the the inode
      // int min_left = inode_left < sector_left ? inode_left : sector_left;
      

   

      int min_left = sector_left;
      int chunk_size = size < min_left ? size : min_left;

      // if(size > (inode_length(inode) - offset)){
      //   allocate_new_sector(&inode->data, offset);
      // }
    if(size > (inode_length(inode) - offset)){
      // for(unsigned i = 0; i < bytes_to_sectors(size); i++){

      //if the pos is past end of file, need to set offset to position and zero out everything before it
        allocate_new_sector(&(inode->data), offset);
        bytes_expanded += 512;
        bytes_expanded -= sector_left;
        // inode->expanded = true;
      // }
    }
      block_sector_t sector_idx = byte_to_sector (&inode->data, offset);
      // if(size > inode_length(inode) + offset){
      //   allocate_new_sector(&(inode->data), offset);
      //   inode->expanded = true;
      // }
      // sector_idx = byte_to_sector(&inode->data, offset);




      /* Number of bytes to actually write into this sector. */
      //either write rest of file or whatever you can fit into the sector
      //change this to write whatever you can fit into the sector, that way we can restart the loop and re-extend the file
      //we are gonna extend the file 1 sector at a time1

      // min_left = sector_left;
      // chunk_size = size;

      if (chunk_size <= 0)
        break;
      //fill up entire sector of bytes
      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Write full sector directly to disk. */
          block_write (fs_device, sector_idx, buffer + bytes_written);
        }
      else 
        {
          /* We need a bounce buffer. */
          if (bounce == NULL) 
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }

          /* If the sector contains data before or after the chunk
             we're writing, then we need to read in the sector
             first.  Otherwise we start with a sector of all zeros. */
          if (sector_ofs > 0 || chunk_size < sector_left) 
            block_read (fs_device, sector_idx, bounce);
          else
            memset (bounce, 0, BLOCK_SECTOR_SIZE);
          memcpy (bounce + sector_ofs, buffer + bytes_written, chunk_size);
          block_write (fs_device, sector_idx, bounce);
        }

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }
  free (bounce);
  off_t inode_left = (inode->data.length) - offset_;
  if(bytes_written > inode_left ){
    inode->data.length += bytes_written - inode_left;
  }
  block_write(fs_device, inode->data.location, &(inode->data));

  return bytes_written;
}

/** Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode) 
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/** Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode) 
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/** Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  return inode->data.length;
}

// assign 4 helper functions

/** Returns true if INODE is a directory, false otherwise. */
bool
inode_is_dir (const struct inode *inode)
{
  return inode->data.is_dir;
}

/** Returns the sector number of the parent directory of INODE. */
block_sector_t
inode_get_parent (const struct inode *inode)
{
  return inode->data.parent;
}

bool inode_make_parent(block_sector_t parent_sector, block_sector_t child_sector) {
  struct inode *inode = inode_open(child_sector);
  if (inode == NULL)
    return false;

  inode->data.parent = parent_sector;

  // Write the modified inode data back to disk
  block_write(fs_device, inode->sector, &inode->data);

  inode_close(inode);
  return true;
}