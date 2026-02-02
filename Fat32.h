/*
 * fat32.h - FAT32 file system header
 */

#ifndef FAT32_H
#define FAT32_H

#define FAT32_OK        0
#define FAT32_ERROR    -1
#define FAT32_NOT_FOUND -2

int fat32_init(void);
int fat32_read_file(const char* filename, unsigned char* buffer, unsigned int max_size);
void fat32_list_files(void);

#endif
