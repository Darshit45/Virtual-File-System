#ifndef __FS_H_
#define __FS_H_

#include <unistd.h>
#include "disk.h"
#include <string.h>
#define DISK_BLOCKS     5120      /* number of blocks on the disk 5*2^10 */
#define BLOCK_SIZE      4096      /* block size on "disk"    4KB 2^12 */
#define MAX_FILE_COUNT  64
extern struct metadata *meta ;
/*opening  file by string comparision and upadating file control block*/
int fs_open(char *name)
{
  int i = 0;
  for (; i < MAX_FILE_COUNT; i++) {
      if (strcmp(meta->fcb_list[i].file_name, name) == 0) {
          meta->fcb_list[i].is_opened = 1;//setting file attribute to open
          return i;
        }
    }
  return -1;
}
int fs_close(int fildes){
meta->fcb_list[fildes].is_opened = 0;  
return 0;
}
/*
First checking whether same file name exists or not
and next finding free file control block. 
 */
int fs_create(char *name)
{
/* prevent duplicate names*/
  int i = 0;
  for (; i < MAX_FILE_COUNT; i++) {
      if (strcmp(name, meta->fcb_list[i].file_name) == 0)//comparision of file names
        return -1;
    }

  /*find a free File control block from volume control block*/
  i = 0;
  for (; i < MAX_FILE_COUNT; i++) {
      if (meta->vcb.free_fcb[i] == 0)// checking for free fcb from vcb and break  
				    //as soon as there is free fcb
        break;
    }

  /* more than 64 files => error*/
  if (i == MAX_FILE_COUNT)
    return -1;

  
  meta->vcb.free_fcb[i] = 1;// initialize VCB and set it to 1 to
			    //indicate that this fcb is not free 
  strcpy(meta->fcb_list[i].file_name, name);//copying the filename to
					    //file control block

  metadata_rewrite();//updating the superblock with new file
  return 0;

}
/*
Deleting the file by deallocating the
memory allocated for this file.
*/
int fs_delete(char *name){
int i = 0;
  for (; i < MAX_FILE_COUNT; i++) {
      if (strcmp(name, meta->fcb_list[i].file_name) == 0)//searching using  
							//string comparision
        break;
    }
  
  if (i == MAX_FILE_COUNT)// if the filename was not found, return
    return -1;

  
  if(meta->fcb_list[i].is_opened == 0)// must open file before deleting
    return -1;

  // if the file size is 0, there is nothing to do with blocks
  if(meta->fcb_list[i].size != 0) {
      int current_block = meta->fcb_list[i].first_block;//get the first block
      char *buffer = (char*)malloc(BLOCK_SIZE);
      int next_block = current_block;//intialize next_block with current_block
      do {
          current_block = next_block ;
          bread(current_block,buffer);//read current block and get it
					   //stored in buffer
          next_block = get_block_icon(buffer);//getting next block from the last
					     //4 bytes of block
          memset(buffer,0,BLOCK_SIZE);//clearing buffer in which we stored the block
          bwrite(current_block,buffer);//writing nextblock to buffer
          meta->vcb.free_block_count ++;//increasing the free block count

        } while(next_block > 0);//All the above will happen only if nextblock exists.
    }

  // deleting metadata
  meta->vcb.free_fcb[i] = 0;//clearing out space from vcb for that file.
  memset(meta->fcb_list[i].file_name,0,20);//clearing the name 
  meta->fcb_list[i].first_block = 0;//clearing first block
  meta->fcb_list[i].last_block = 0;//clearing last block
  meta->fcb_list[i].last_block_used = 0;//clearing out last_block_used
  meta->fcb_list[i].size = 0;//clearing size

  metadata_rewrite();//updating superblock

}
int fs_read(int fildes, void *buffer, size_t nbyte){
// check if the file is opened
  if (meta->fcb_list[fildes].is_opened == 0)
    return -1;

  // if want to read more than file's size, replace
  // read size with file's size to prevent further
  // errors
  if (meta->fcb_list[fildes].size < nbyte)
    nbyte = meta->fcb_list[fildes].size;


  int block = meta->fcb_list[fildes].first_block;//get the first block

  if (block == 0)
    return -1;

  char *temp = (char*)malloc(BLOCK_SIZE);

  int i = 0;
  // iterate through the linked-list of blocks
  // and each time, read the block, and copy to buffer
  while (nbyte > BLOCK_SIZE - 4) {
      bread(block, temp);//if file size is more than one block
			    //we have to copy until file size is less than
			    //BLOCK_SIZE-4 (-4) is because i am storing 
			   //integer at the  at the end of every block
      memcpy(buffer + (i * (BLOCK_SIZE - 4)), temp, BLOCK_SIZE - 4);
		//copying the temp to buffer
      block = get_block_icon(temp);//getting the next block
      nbyte -= BLOCK_SIZE - 4;//we have already the BLOCK SIZE data to buffer
      i++;
    }
memset(temp, 0, BLOCK_SIZE);
  bread(block, temp);//reading the data which is less than BLOCK_SIZE-4
  memcpy(buffer + (i * (BLOCK_SIZE - 4)), temp, nbyte);//copying it to buffer
  return 0;

}
int get_block_icon(char *buffer)
{
  return * ((int *) (buffer + BLOCK_SIZE - 4));//get the next block number by
						//accessing the last 4  
}


int get_first_empty_block_from(int i)
{
  char *buffer = (char *) malloc(BLOCK_SIZE);

  for (; i < DISK_BLOCKS; i++) {//searching amonng all the blocks
      bread(i, buffer);	//and accessing to check whether they are free
      if (get_block_icon(buffer) == 0) {//It is not allocated to any file
          free(buffer);//free the buffer
          return i;// return the block number of free block
        }
    }
  free(buffer);
  return -1;
}

int get_first_empty_block()
{
  return get_first_empty_block_from(1);//wrapper function to get the 
					//first empty block
}
int write_starting_from_block(int last, char *buffer, size_t *n_byte)
{
  int next, nbyte = *n_byte;
  char *buff = (char *) malloc(BLOCK_SIZE);

  int i = 0,enter=0;
  // write in (BLOCK_SIZE - 4) chunks
  while (nbyte > BLOCK_SIZE - 4) {
      next = get_first_empty_block_from(last + 1);
      memcpy(buff, buffer + (i * (BLOCK_SIZE- 4)), BLOCK_SIZE - 4);
	//copy the data from buffer to buff
      memcpy(buff + ((i + 1) * BLOCK_SIZE - 4), &next, 4);
	//write the next  block number which we got from 
	//get_first_emptyb_lock
      bwrite(last, buff);
//write the data from buff to blockno last
      last = next;
      nbyte -= (BLOCK_SIZE - 4);//reduce the amount of data need to be written
      i++;
meta->vcb.free_block_count=meta->vcb.free_block_count-1;
//decrease the free block count
enter=1;
    }
if(enter==0)
meta->vcb.free_block_count=meta->vcb.free_block_count-1;
  // write the remainder data (less than BLOCK_SIZE - 4)
  next = -1;
  memset(buff, 0, BLOCK_SIZE);

  memcpy(buff, buffer + (i * (BLOCK_SIZE - 4)), nbyte);
  memcpy(buff + ((i + 1) * BLOCK_SIZE - 4), &next, 4);
//printf("data is%s\n",buff);
  bwrite(last, buff);//write the leftover portion to the block

  return last;
}
int fs_write(int fildes, void *buffer, size_t nbyte)
{
// check file is opened
  if (meta->fcb_list[fildes].is_opened == 0)
    return -1;

  // check that there is free space left on hard-disk
  if (meta->vcb.free_block_count < (nbyte / BLOCK_SIZE) + 1)
    return -1;

  // update size of FCB
  meta->fcb_list[fildes].size += nbyte;

  // if this is the first time to write to file
  // then we must find an empty block and write to it
  if (meta->fcb_list[fildes].first_block == 0) {
      int last = get_first_empty_block();//get the first empty block
	//printf("block no:%d\n",last);
      meta->fcb_list[fildes].first_block = last;//update the fiel control block
						//with first block number
      last = write_starting_from_block(last, buffer, &nbyte);
						//Get the last blocknumber for that file
      meta->fcb_list[fildes].last_block = last;
      meta->fcb_list[fildes].last_block_used = nbyte;
					//update the used size in the last block
    }
  // else we must find last block of previous write
  else {
      char *buff = (char *) malloc(BLOCK_SIZE);

      // if the new data does not need allocation of new block
      if (meta->fcb_list[fildes].last_block_used + 4 + nbyte <= BLOCK_SIZE) {
          bread(meta->fcb_list[fildes].last_block, buff);//Here we are
				//adding the content in  the last block to buff
						
          memcpy(buff + (meta->fcb_list[fildes].last_block_used), buffer, nbyte);//here the new data is appended to the buff
				 
          bwrite(meta->fcb_list[fildes].last_block, buff);//writing the buff to block
				
          meta->fcb_list[fildes].last_block_used += nbyte;//updating the used size of last block
				 
        }
      // there are new blocks to be assigned to the file
      else {
          int last = meta->fcb_list[fildes].last_block;//get the last block assigned for the file.

          bread(last, buff);//read the last block and store it in buff

//If there is some space left in the last block than get that much amount of data from buffer
//and write it to buff
          memcpy(buff + (meta->fcb_list[fildes].last_block_used),
                 buffer,
                 (BLOCK_SIZE - 4 - meta->fcb_list[fildes].last_block_used));
		
          int next = get_first_empty_block();//get the next free block
          memcpy(buff + (BLOCK_SIZE - 4), &next, 4);//update next block in the last block to
						//next free block
          bwrite(last, buff);//write the buff to the lastblock(unused sapce)

          last = next;//update last block to new block
          nbyte -= (BLOCK_SIZE - 4 - meta->fcb_list[fildes].last_block_used);
			//calculate how much data need to be written
          last = write_starting_from_block(last, buffer, &nbyte);
				//write the remaining data using write_starting_from_block
				//If it requires more than one block write_starting_from_block
				//does it
          meta->fcb_list[fildes].last_block = last;//update the last block
          meta->fcb_list[fildes].last_block_used = nbyte;//update the used size of lastblock
        }
    }
  metadata_rewrite();//update the super block

}
#endif
