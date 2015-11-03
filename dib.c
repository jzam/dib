/*
 * Disk Image Browser Project
 * dib.c
 *
 * This program duplicates the contents of a given disk image
 */
#include <tsk/libtsk.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dib.h"

/* The main
 * Parameters: [executable] [disk image] [name of output folder containing disk image contents] 
 */
int main(int argc, char **argv) {
   TSK_IMG_TYPE_ENUM imgType = TSK_IMG_TYPE_DETECT;
   TSK_VS_TYPE_ENUM vsType = TSK_VS_TYPE_DETECT;
   TSK_FS_TYPE_ENUM fsType = TSK_FS_TYPE_DETECT;
   
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
   char *rootName;
   char *ext;
   ssize_t bytesRead = 0;
   FILE *fp;
   size_t check;

   //Verify correct number of arguments
   if (argc != NUM_ARGS) {
      printf("ERROR: Incorrect number of arguments given.\n");
      return ERROR;
   }

   //Disk image
   diskPath = argv[1];

   //Name of output root folder
   rootName = argv[2];

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

   //Copy all of the files
   copyAllFiles(partList, rootName);
   
   //Close disk objects
   tsk_vs_close(vs);
   tsk_img_close(disk);
}

//Construct list of used partitions, so far this program has only been tested with single-partition disks...
void buildPartList(TSK_VS_PART_INFO *allPartList, TSK_VS_PART_INFO **partListHead) {
   int i = 1;
   TSK_VS_PART_INFO *partList = NULL;
   while (allPartList) {
      /* testing printf's
      printf("Partition #%d:\n", i++);
      printf("Address: %d\n", allPartList->addr);
      printf("Description: %s\n", allPartList->desc);
      printf("Num Sectors: %d\n", (int)allPartList->len);
      printf("Sector Offset: %d\n", (int)allPartList->start);
      */
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

//Print directory using path to directory or its inum -- for testing
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

   //Prints all files in directory including inum -- testing
   /*
   for (i = 0; i < dir->names_used; i++) {
      printf("%s - inum: %d\n", dir->names[i].name, (int)dir->names[i].meta_addr);
   }
   */

   //Close fs objects
   tsk_fs_dir_close(dir);
   tsk_fs_close(fs);
}

/* Copy a file from disk image to same relative location in the copy of the
 * disk image
 */
void copyFile(TSK_FS_INFO *fs, char *filePath, char *fileName) {
   TSK_FS_FILE *file;
   TSK_FS_DIR *dir;
   FILE *fp;
   char *buf;
   char *fullPath;
   char *newPath;
   int i;
   int err;
   ssize_t bytesRead;
   size_t check;

   if (filePath == NULL)
      printf("EMPTY file path\n");
   if (fileName == NULL)
      printf("EMPTY file Name\n");

   fullPath = malloc(MAX_PATH);
   strcpy(fullPath, filePath);

   //File path with '/' appended for creating correct path names
   if (strcmp(fullPath, "/"))
      strcat(fullPath, "/");

   file = tsk_fs_file_open(fs, NULL, strcat(fullPath, fileName));

   if (file == NULL) 
      printf("Problem opening file: %s\n", fullPath);

   newPath = &(fullPath[1]);

   if (file->meta->type == TSK_FS_META_TYPE_DIR) {

      dir = tsk_fs_dir_open(fs, fullPath);
      if (!dir)
         printf("ERROR: Could not open directory\n");

      err = mkdir(newPath, S_IRWXU | S_IRWXG | S_IRWXO);
   
      if (err < 0)
         printf("ERROR: Could not make directory\n");

      //Recursive call to copy files in this directory
      for (i = 0; i < dir->names_used; i++) {
         if (strcmp(".", dir->names[i].name) && strcmp("..", dir->names[i].name)) {
            //printf("%s\n", dir->names[i].name);
            copyFile(fs, fullPath, dir->names[i].name);
         }
      }
   }
   else {
      //Allocate memory to hold file contents
      buf = malloc(file->meta->size);

      //Read contents of disk file into buffer
      bytesRead = tsk_fs_file_read(file, 0, buf, file->meta->size, TSK_FS_FILE_READ_FLAG_SLACK);
   
      //Create a new file
      fp = fopen(newPath, "w");

      //Write contents into file
      check = fwrite(buf, bytesRead, 1, fp);
      if (check < 0)
         printf("ERROR: Could not write file\n");

      //Close the new file
      fclose(fp);

      //Free memory  
      free(buf);

      //Close file object
      tsk_fs_file_close(file);
   }
}

/* Copies all of the files of the disk image by calling copyFile for each 
 * file on the disk
 */
void copyAllFiles(TSK_VS_PART_INFO *vs, char *rootName) {
   TSK_FS_INFO *fs;
   TSK_FS_DIR *dir;
   TSK_FS_FILE *root;
   TSK_INUM_T inum;
   FILE *fp;
   int i;   
   int err;
   char *buf;
   char *filePath = malloc(MAX_PATH);
   ssize_t bytesRead;
   size_t check;

   //Open file system object
   fs = tsk_fs_open_vol(vs, TSK_FS_TYPE_DETECT);

   //Set file path to root
   strcpy(filePath, "/");

   if (fs == NULL) {
      printf("ERROR: Could not open filesystem.\n");
   }

   dir = tsk_fs_dir_open(fs, "");
   if (!dir)
      printf("ERROR: Could not open root directory\n");

   err = mkdir(rootName, S_IRWXU | S_IRWXG | S_IRWXO);
   
   if (err < 0)
      printf("ERROR: Could not make directory\n");

   err = chdir(rootName);
   if (err < 0)
      printf("ERROR: Could not change directory\n");

   //Prints all files in directory including inum
   for (i = 0; i < dir->names_used; i++) {
      if (strcmp(".", dir->names[i].name) && strcmp("..", dir->names[i].name)) {
         //printf("%s\n", dir->names[i].name);
         copyFile(fs, filePath, dir->names[i].name);
      }
   }

   //Close directory
   tsk_fs_dir_close(dir);
}
