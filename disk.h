#ifndef _DISK_H_
#define _DISK_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define DISK_BLOCKS     5120      /* number of blocks on the disk 5*2^10   */
#define BLOCK_SIZE      4096      /* block size on "disk"    4KB      2^12 */
#define MAX_FILE_COUNT  64	 /*maximim files on a disk*/

/**
 * This FCB(file control block) struct contains all the attributes for the file.
 */
struct fcb {
  char file_name[20];//what is the name of file
  unsigned char is_opened;//to see whether it is opened or not
  unsigned int size;//to check how much size this file has occupied
  unsigned int first_block;//get the first block for the file
  unsigned int last_block;//get the last block assigned for the file
  unsigned int last_block_used; // n-bytes used from last block
} __attribute__((packed));

/**
 * This VCB (volume control block) struct contains how many free blocks were there
 *  and also to check file fcb is free. 
 */
struct vcb {
  int free_block_count;//calculating free block count
  unsigned char free_fcb[MAX_FILE_COUNT];//to see whether there are any free fcb 
} __attribute__((packed));
/*Meta data (super block) has all the information about volume control blocks and 
*and informaion about each file using array of fcb's.
*/
struct metadata {
  int disk_blocks;  // for validation
  struct vcb vcb;  //for checking free fcb
  struct fcb fcb_list[MAX_FILE_COUNT];//for storing attributes of every file
} __attribute__((packed));

static int active = 0;  /* is the virtual disk open (active) */
static int handle;      /* file handle to virtual disk       */
static char open_disk_name[128];

struct metadata *meta;
/*making disk of 32MB*/
int make_disk(char *name)
{ 
  int f, cnt;
  char buf[BLOCK_SIZE];

  if (!name) {
    fprintf(stderr, "make_disk: invalid file name\n");
    return -1;
  }

  if ((f = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
    perror("make_disk: cannot open file");
    return -1;
  }

  memset(buf, 0, BLOCK_SIZE);
  for (cnt = 0; cnt < DISK_BLOCKS; ++cnt)
    write(f, buf, BLOCK_SIZE);

  close(f);

  return 0;
}
/*opening disk*/
int open_disk(char *name)
{
  int f;

  if (!name) {
    fprintf(stderr, "open_disk: invalid file name\n");//invalid file name
    return -1;
  }  
  
  if (active) {
    fprintf(stderr, "open_disk: disk is already open\n");//if disk is open no need
    return -1;//of opening it again
  }
  
  if ((f = open(name, O_RDWR, 0644)) < 0) {
    perror("open_disk: cannot open file");
    return -1;
  }

  handle = f;//giving file descriptor to handle
  active = 1;

  return 0;
}
/*closing disk by closing handle*/
int close_disk()
{
  if (!active) {
    fprintf(stderr, "close_disk: no open disk\n");//close only if disk is open
    return -1;
  }
  
  close(handle);//closing handle

  active = handle = 0;

  return 0;
}
int bwrite(int block, char *buf)
{
  if (!active) {
    fprintf(stderr, "bwrite: disk not active\n");//disk not active
    return -1;
  }

  if ((block < 0) || (block >= DISK_BLOCKS)) {//accesssing blocks out of range
    fprintf(stderr, "bwrite: block index out of bounds\n");
    return -1;
  }

  if (lseek(handle, block * BLOCK_SIZE, SEEK_SET) < 0) {//pointing to given block
    perror("bwrite: failed to lseek");
    return -1;
  }

  if (write(handle, buf, BLOCK_SIZE) < 0) {//writing to the file using handler
    perror("bwrite: failed to write");
    return -1;
  }

  return 0;
}
int bread(int block, char *buf)
{
  if (!active) {
    fprintf(stderr, "bread: disk not active\n");//disk not active
    return -1;
  }

  if ((block < 0) || (block >= DISK_BLOCKS)) {
    fprintf(stderr, "bread: block index out of bounds\n");//diskout of bounds
    return -1;
  }

  if (lseek(handle, (block) * BLOCK_SIZE, SEEK_SET) < 0) {//setting exactly to block 
    perror("bread: failed to lseek");//which you want to read
    return -1;
  }

  if (read(handle, buf, BLOCK_SIZE) < 0) {//reading it using handle
    perror("bread: failed to read");//error failed to read
    return -1;
  }
//printf("%s",buf);
  return 0;
}
int make_fs(char *disk_name)
{
  // check file existance
  if (access(disk_name, F_OK) != -1)
    return -1;

  if (make_disk(disk_name) == 0) {
     
      struct vcb _vcb; // initialize a VCB
      memset(&_vcb, 0, sizeof(struct vcb));//clearing out vcb and setting to 0
      _vcb.free_block_count = DISK_BLOCKS;//setting up free block count to default

      struct metadata _meta;//intialize metadata
      memset(&_meta, 0, sizeof(struct metadata));//clearing out vcb and setting to 0
      _meta.vcb = _vcb;//in the meta intializing vcb
      _meta.disk_blocks = DISK_BLOCKS;//intializing free blocks

      void *temp = malloc(BLOCK_SIZE);//creating a block size
      memset(temp, 0, BLOCK_SIZE);//setting temp  to 0
      memcpy(temp, &_meta, sizeof(struct metadata));//copied to temp from meta

      open_disk(disk_name);//opening disk
      bwrite(0, temp);//Setting up super block
			//by writing the meta data to block 0
      close_disk(disk_name);//closing disk 

      return 0;
    }
  return -1;
}
int mount_fs(char *disk_name)
{
  if (active != 0)
    return -1;
  if (open_disk(disk_name) == -1)
    return -1;

  void *temp = malloc(BLOCK_SIZE);
  memset(temp, 0, BLOCK_SIZE);

  // read metadata from disk and put it in RAM
  bread(0, temp);
	
  meta = (struct metadata *) temp;

  // validation for checking true filesystem
  if (meta->disk_blocks != DISK_BLOCKS)
    return -1;

  strcpy(open_disk_name, disk_name);

  return 0;
}
int umount_fs(char *disk_name)
{
  if (active != 1)
    return -1;
  if (strcmp(open_disk_name, disk_name) != 0)
    return -1;

  if (metadata_rewrite() == -1)//writing back the the data
    return -1;

  if (close_disk() == -1)
    return -1;

  return 0;
}
int metadata_rewrite()
{
  void *temp = malloc(BLOCK_SIZE);
  memset(temp, 0, BLOCK_SIZE);
  memcpy(temp, meta, sizeof(struct metadata));//copying metadata to temp

  return bwrite(0, temp);//writing it into 0th block(super block)
}


#endif
