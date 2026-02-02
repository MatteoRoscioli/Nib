/*
 * fat32.c - Minimal FAT32 file system implementation
 */

#include "fat32.h"
#include "sd.h"
#include "uart.h"
#include "memory.h"

// FAT32 structures
typedef struct {
    unsigned char  jmp[3];
    unsigned char  oem[8];
    unsigned short sector_size;
    unsigned char  sectors_per_cluster;
    unsigned short reserved_sectors;
    unsigned char  fat_count;
    unsigned short root_entries;
    unsigned short total_sectors_short;
    unsigned char  media_type;
    unsigned short fat_size_16;
    unsigned short sectors_per_track;
    unsigned short head_count;
    unsigned int   hidden_sectors;
    unsigned int   total_sectors;
    unsigned int   fat_size_32;
    unsigned short flags;
    unsigned short version;
    unsigned int   root_cluster;
    unsigned short fsinfo_sector;
    unsigned short backup_boot;
    unsigned char  reserved[12];
    unsigned char  drive_number;
    unsigned char  reserved1;
    unsigned char  boot_signature;
    unsigned int   volume_id;
    unsigned char  volume_label[11];
    unsigned char  fs_type[8];
} __attribute__((packed)) fat32_boot_sector_t;

typedef struct {
    unsigned char  name[11];
    unsigned char  attributes;
    unsigned char  reserved;
    unsigned char  creation_time_fine;
    unsigned short creation_time;
    unsigned short creation_date;
    unsigned short access_date;
    unsigned short cluster_high;
    unsigned short modified_time;
    unsigned short modified_date;
    unsigned short cluster_low;
    unsigned int   file_size;
} __attribute__((packed)) fat32_dir_entry_t;

static fat32_boot_sector_t boot_sector;
static unsigned int fat_start;
static unsigned int data_start;
static unsigned char sector_buffer[512];

int fat32_init(void) {
    uart_puts("Initializing FAT32 file system...\n");
    
    // Read boot sector
    if (sd_read_block(0, sector_buffer) != SD_OK) {
        uart_puts("FAT32: Failed to read boot sector\n");
        return FAT32_ERROR;
    }
    
    // Copy boot sector
    memcpy(&boot_sector, sector_buffer, sizeof(fat32_boot_sector_t));
    
    // Verify FAT32
    if (boot_sector.fs_type[0] != 'F' || 
        boot_sector.fs_type[1] != 'A' ||
        boot_sector.fs_type[2] != 'T') {
        uart_puts("FAT32: Not a FAT file system\n");
        return FAT32_ERROR;
    }
    
    // Calculate important values
    fat_start = boot_sector.reserved_sectors;
    data_start = fat_start + (boot_sector.fat_count * boot_sector.fat_size_32);
    
    uart_puts("FAT32: Initialized successfully\n");
    uart_puts("  Sector size: ");
    uart_dec(boot_sector.sector_size);
    uart_puts("\n  Sectors per cluster: ");
    uart_dec(boot_sector.sectors_per_cluster);
    uart_puts("\n  FAT start: ");
    uart_dec(fat_start);
    uart_puts("\n  Data start: ");
    uart_dec(data_start);
    uart_puts("\n");
    
    return FAT32_OK;
}

static unsigned int cluster_to_sector(unsigned int cluster) {
    return data_start + ((cluster - 2) * boot_sector.sectors_per_cluster);
}

static unsigned int get_next_cluster(unsigned int cluster) {
    unsigned int fat_offset = cluster * 4;
    unsigned int fat_sector = fat_start + (fat_offset / 512);
    unsigned int entry_offset = fat_offset % 512;
    
    if (sd_read_block(fat_sector, sector_buffer) != SD_OK) {
        return 0;
    }
    
    return *(unsigned int*)(sector_buffer + entry_offset) & 0x0FFFFFFF;
}

int fat32_read_file(const char* filename, unsigned char* buffer, unsigned int max_size) {
    uart_puts("Reading file: ");
    uart_puts(filename);
    uart_puts("\n");
    
    // Read root directory
    unsigned int root_sector = cluster_to_sector(boot_sector.root_cluster);
    
    if (sd_read_block(root_sector, sector_buffer) != SD_OK) {
        uart_puts("FAT32: Failed to read root directory\n");
        return FAT32_ERROR;
    }
    
    // Convert filename to FAT format (8.3)
    char fat_name[11];
    memset(fat_name, ' ', 11);
    
    int i = 0, j = 0;
    while (filename[i] && filename[i] != '.' && j < 8) {
        fat_name[j++] = filename[i] >= 'a' ? filename[i] - 32 : filename[i];
        i++;
    }
    
    if (filename[i] == '.') {
        i++;
        j = 8;
        while (filename[i] && j < 11) {
            fat_name[j++] = filename[i] >= 'a' ? filename[i] - 32 : filename[i];
            i++;
        }
    }
    
    // Search for file
    fat32_dir_entry_t* entries = (fat32_dir_entry_t*)sector_buffer;
    int found = 0;
    unsigned int file_cluster = 0;
    unsigned int file_size = 0;
    
    for (int e = 0; e < 16; e++) {
        if (entries[e].name[0] == 0) break;
        if (entries[e].name[0] == 0xE5) continue;
        
        int match = 1;
        for (int n = 0; n < 11; n++) {
            if (entries[e].name[n] != fat_name[n]) {
                match = 0;
                break;
            }
        }
        
        if (match) {
            found = 1;
            file_cluster = ((unsigned int)entries[e].cluster_high << 16) | entries[e].cluster_low;
            file_size = entries[e].file_size;
            break;
        }
    }
    
    if (!found) {
        uart_puts("FAT32: File not found\n");
        return FAT32_NOT_FOUND;
    }
    
    uart_puts("Found file, size: ");
    uart_dec(file_size);
    uart_puts(" bytes\n");
    
    if (file_size > max_size) {
        uart_puts("FAT32: File too large\n");
        return FAT32_ERROR;
    }
    
    // Read file data
    unsigned int bytes_read = 0;
    unsigned int cluster = file_cluster;
    
    while (cluster < 0x0FFFFFF8 && bytes_read < file_size) {
        unsigned int sector = cluster_to_sector(cluster);
        
        for (int s = 0; s < boot_sector.sectors_per_cluster; s++) {
            if (bytes_read >= file_size) break;
            
            if (sd_read_block(sector + s, sector_buffer) != SD_OK) {
                uart_puts("FAT32: Failed to read file data\n");
                return FAT32_ERROR;
            }
            
            unsigned int to_copy = file_size - bytes_read;
            if (to_copy > 512) to_copy = 512;
            
            memcpy(buffer + bytes_read, sector_buffer, to_copy);
            bytes_read += to_copy;
        }
        
        cluster = get_next_cluster(cluster);
    }
    
    uart_puts("Read ");
    uart_dec(bytes_read);
    uart_puts(" bytes\n");
    
    return bytes_read;
}

void fat32_list_files(void) {
    uart_puts("\nFiles in root directory:\n");
    uart_puts("========================\n");
    
    // Read root directory
    unsigned int root_sector = cluster_to_sector(boot_sector.root_cluster);
    
    if (sd_read_block(root_sector, sector_buffer) != SD_OK) {
        uart_puts("FAT32: Failed to read root directory\n");
        return;
    }
    
    fat32_dir_entry_t* entries = (fat32_dir_entry_t*)sector_buffer;
    
    for (int e = 0; e < 16; e++) {
        if (entries[e].name[0] == 0) break;
        if (entries[e].name[0] == 0xE5) continue;
        if (entries[e].attributes & 0x08) continue; // Skip volume label
        
        // Print filename
        for (int i = 0; i < 8; i++) {
            if (entries[e].name[i] != ' ') {
                uart_putc(entries[e].name[i]);
            }
        }
        
        if (entries[e].name[8] != ' ') {
            uart_putc('.');
            for (int i = 8; i < 11; i++) {
                if (entries[e].name[i] != ' ') {
                    uart_putc(entries[e].name[i]);
                }
            }
        }
        
        uart_puts("  (");
        uart_dec(entries[e].file_size);
        uart_puts(" bytes)\n");
    }
    
    uart_puts("========================\n");
}
