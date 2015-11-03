/*
 * Disk Image Browser Project
 * dib.h
 *
 * Header file for dib.c
 */

#define NUM_ARGS 3
#define ERROR -1
#define SUCCESS 0
#define MAX_PATH 260

extern void buildPartList(TSK_VS_PART_INFO *allPartList, TSK_VS_PART_INFO **partListHead);
extern void printDirectory(TSK_VS_PART_INFO *vsInfo, char *dirPath, TSK_INUM_T inum);
extern void copyFile(TSK_FS_INFO *fs, char *filePath, char *fileName);
extern void copyAllFiles(TSK_VS_PART_INFO *vs, char *rootName);

