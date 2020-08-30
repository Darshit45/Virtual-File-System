#include "disk.h"
#include "fs.h"
#include <stdio.h>
extern struct metadata *meta;
int main() {
printf("make:  %d\n", make_fs("test_disk.fs"));//making disk
printf("mount: %d\n", mount_fs("test_disk.fs"));//mounting disk
int fg=0;
/*creating ram.txt ,opening and writing into it*/
fg=fs_create("ram.txt");
if(fg==0)
printf("fs_create: file successfully created\n");
else
printf("already created\n");
int filedes = fs_open("ram.txt");
if(filedes>=0)
printf("fs_open:  file succesfully opened with fd: %d\n", filedes);
else
printf("unable to open");
fg=fs_write(filedes, "Hi ram krishna",15 );
if(fg==0)
printf("fs_write: file successfully written\n");
else
printf("file error\n");


/*creating ketan.txt ,opening and writing into it*/
fg=fs_create("ketan.txt");
if(fg==0)
printf("fs_create: file successfully created\n");
else
printf("already created\n");
int filedes2 = fs_open("ketan.txt");
if(filedes>=0)
printf("fs_open:  file succesfully opened with fd: %d\n", filedes2);
else
printf("unable to open");
fg=fs_write(filedes2, "Hi ketan ki", 11);
if(fg==0)
printf("fs_write: file successfully written\n");
else
printf("file error\n");


/*Reading data from ram.text*/
 char *buffer = (char *) malloc(200);
fg=fs_read(filedes, buffer, 200);
if(fg==0)
printf("fs_read:   file successfully read\n");//reading from buffer
else
printf("error");
printf("read: %s\n", buffer);//printing the data in the buffer

/*Reading data from ketan.txt*/
memset(buffer, 0,200);
fg=fs_read(filedes2, buffer, 200);
if(fg==0)
printf("fs_read:   file successfully read\n");//reading from buffer
else
printf("error");
printf("read: %s\n", buffer);//printing the data in the buffer

/*printing the number of free blocks*/
printf("No of free blocks: %d\n",meta->vcb.free_block_count);

/*deleting ketan.txt*/
printf("fs_delete: %d\n",fs_delete("ketan.txt"));

/*printing the number of free blocks after deletion*/
printf("No of free blocks: %d\n",meta->vcb.free_block_count);

/*printing the files on disk and showing
what are the bolocks assigned to it*/
printf("files on disk:\n");

int i=0;
while(i<64){   //Here we are traversing every file control block
	if(meta->vcb.free_fcb[i] ==1){ //Here we are traversing volume control block 
					//to check whether there are any files or not 
		printf("%s\n",meta->fcb_list[i].file_name);//printing the files on disk
		int firstblock=meta->fcb_list[i].first_block;// accessing first block from file 
							    //control block of a particular file
		int num=firstblock,temp=0;
		char *buff = (char *) malloc(BLOCK_SIZE);
		printf("size of %s %d\n",meta->fcb_list[i].file_name,meta->fcb_list[i].size);//printing the size of file
		printf("blocks assiged to this file are: ");//Here we are checking what blocks
							   //has been assigned to this file
			while(num!=-1){
			bread(num, buff);	//Here we are reading the block 
			temp=get_block_icon(buff);	//Getting the block no of nextblock
							//next block number is stored in every block at the end
			printf("%d ",num);		//printing the blockno
			num=temp;			//checking whether there is next block or not
							//if num==-1 then there is no next block
			}
					}
	else{//No files has been assigned after this i
	break;// so we are breaking from loop
	}
	i++;
printf("\n");
}

printf("\n");
printf("fs_close:  %d\n", fs_close(filedes)); //closing the file ram.txt
printf("fs_close:  %d\n", fs_close(filedes2)); //closing the file ketan.txt
printf("umount:%d\n", umount_fs("test_disk.fs")); //un mounting the disk

  return 0;
}
