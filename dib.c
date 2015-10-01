#include <tsk/libtsk.h>
#include <stdio.h>
#include <stdlib.h>
#define NUM_ARGS 4
#define ERROR -1
#define SUCCESS 0

//Construct list of used partitions, so far this program has only been tested with single-partition disks...
void buildPartList(TSK_VS_PART_INFO *allPartList, TSK_VS_PART_INFO **partListHead) {
   int i = 1;
   TSK_VS_PART_INFO *partList = NULL;
   while (allPartList) {
      printf("Partition #%d:\n", i++);
      printf("Address: %d\n", allPartList->addr);
      printf("Description: %s\n", allPartList->desc);
      printf("Num Sectors: %d\n", (int)allPartList->len);
      printf("Sector Offset: %d\n", (int)allPartList->start);
      if (allPartList->slot_num >= 0) {
         if (*partListHead == NULL) {
            partList = *partListHead = allPartList;
         }
         else {
            partList->next = allPartList;
            partList->next->prev = partList;
            partList = partList->next;
         }
      }
      allPartList = allPartList->next;    
   }
}

//Print directory using path to directory or its inum
void printDirectory(TSK_VS_PART_INFO *vsInfo, char *dirPath, TSK_INUM_T inum) {
   TSK_FS_INFO *fs;
   TSK_FS_DIR *dir;
   int i;   

   //Open file system object
   fs = tsk_fs_open_vol(vsInfo, TSK_FS_TYPE_DETECT);

   if (fs == NULL) {
      printf("Could not open fs. :(\n");
   }

   //Open directory object with directory path or inum
   if (dirPath != NULL) {
      dir = tsk_fs_dir_open(fs, dirPath);
      if (dir == NULL) {
         printf("ERROR: Attempt to open non-directory @ %s\n", dirPath);
         exit(0);
      }
   }
   else {
      dir = tsk_fs_dir_open_meta(fs, inum);
      if (dir == NULL) {
         printf("ERROR: Attempt to open non-directory @ inum %d\n", (int) inum);
         exit(0);
      }
   }

   //Prints all files in directory including inum
   for (i = 0; i < dir->names_used; i++) {
      printf("%s - inum: %d\n", dir->names[i].name, (int)dir->names[i].meta_addr);
   }

   //Close fs objects
   tsk_fs_dir_close(dir);
   tsk_fs_close(fs);
}

//Copies file from disk image to an output file - Hardcoded for testing
void copyFile(TSK_VS_PART_INFO *vsInfo, char *dirPath, TSK_INUM_T inu, char *fileName) {
   TSK_FS_INFO *fs;
   TSK_FS_DIR *dir;
   TSK_FS_FILE *file;
   TSK_INUM_T inum;
   FILE *fp;
   int i;   
   char *buf;
   ssize_t bytesRead;
   size_t check;

   //Open file system object
   fs = tsk_fs_open_vol(vsInfo, TSK_FS_TYPE_DETECT);

   if (fs == NULL) {
      printf("Could not open fs. :(\n");
   }

   //Open directory object with directory path or inum
   if (dirPath != NULL) {
      dir = tsk_fs_dir_open(fs, dirPath);
      if (dir == NULL) {
         printf("ERROR: Attempt to open non-directory @ %s\n", dirPath);
         exit(0);
      }
   }
   else {
      dir = tsk_fs_dir_open_meta(fs, inu);
      if (dir == NULL) {
         printf("ERROR: Attempt to open non-directory @ inum %d\n", (int) inu);
         exit(0);
      }
   }

   inum = dir->names[2].meta_addr;
   printf("inum is %d\n", (int)inum);
   file = tsk_fs_file_open_meta(fs, NULL, inum);
   if (file == NULL) 
      printf("!File is NULL!\n");
   
   //Allocate memory to hold file contents
   buf = malloc(file->meta->size);

   //printf("Values passed to file read function: Off: %d, Size: %d\n", (int)file->fs_info->offset, (int)file->meta->size); 
   bytesRead = tsk_fs_file_read(file, 0, buf, file->meta->size, TSK_FS_FILE_READ_FLAG_SLACK);
   
   /*printf("Offset: %d, Size: %d, Flags: %0x\n", (int)file->fs_info->offset, (int)file->meta->size, (int)file->meta->flags);

   if (bytesRead < 0)
      printf("ERROR reading file\n");
   else
      printf("Read %d bytes\n", (int)bytesRead);
   */
   fp = fopen(fileName, "w");
   check = fwrite(buf, bytesRead, 1, fp);
   if (check <= 0)
      printf("ERROR writing file\n");
   fclose(fp);  
   free(buf);
}

int main(int argc, char **argv) {
   TSK_IMG_TYPE_ENUM imgType = TSK_IMG_TYPE_DETECT;
   TSK_VS_TYPE_ENUM vsType = TSK_VS_TYPE_DETECT;
   TSK_FS_TYPE_ENUM fsType = TSK_FS_TYPE_DETECT;

   TSK_FS_DIR_WALK_FLAG_ENUM dirWalkFlag = TSK_FS_DIR_WALK_FLAG_NONE;
   
   TSK_IMG_INFO *disk;
   TSK_VS_INFO *vs;
   TSK_VS_PART_INFO *allPartList;
   TSK_VS_PART_INFO *partList = NULL;
   TSK_FS_DIR *dir;
   TSK_FS_INFO *fs;
   TSK_INUM_T inum;
   TSK_FS_FILE *file = NULL;

   unsigned int blockSize;
   int i;
   int inu;
   char c;
   char *diskPath;
   char *dirPath;
   char *buf;
   ssize_t bytesRead = 0;
   FILE *fp;
   size_t check;

   printf("MALLOC ATTEMPT\n");
   dirPath = malloc(3452);

   //Verify correct number of arguments
   if (argc != NUM_ARGS) {
      printf("ERROR: Incorrect number of arguments given.\n");
      return ERROR;
   }
   diskPath = argv[1];
   c = *argv[2];
   if (c == 'p') {
      inu = 0;
      dirPath = argv[3];
   }
   else {
      dirPath = NULL;
      inu = atoi(argv[3]);
   }

   //Open the disk image
   disk = tsk_img_open_sing(diskPath, imgType, 0);
   if (disk == NULL) {
      printf("ERROR: Failed to open disk image @ %s\n", diskPath);
      return ERROR;
   }

   //Get info about partitions
   vs = tsk_vs_open(disk, 0, TSK_VS_TYPE_DETECT);
   allPartList = vs->part_list;

   //Build list of partitions with valid file systems
   buildPartList(allPartList, &partList);

   //Print input directory
   printDirectory(partList, dirPath, inu);

   //Copy file to output - Hardcoded for testing right now
   copyFile(partList, dirPath, inu, "out.txt");

   //Close disk objects
   tsk_vs_close(vs);
   tsk_img_close(disk);
}
