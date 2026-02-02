/*
 * sd.h - SD Card driver header
 */

#ifndef SD_H
#define SD_H

#define SD_OK       0
#define SD_ERROR   -1
#define SD_TIMEOUT -2

int sd_init(void);
int sd_read_block(unsigned int block, unsigned char* buffer);
int sd_write_block(unsigned int block, const unsigned char* buffer);

#endif
