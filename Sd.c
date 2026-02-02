/*
 * sd.c - SD Card driver for Raspberry Pi
 */

#include "sd.h"
#include "uart.h"

// EMMC registers (Raspberry Pi 2/3)
#define EMMC_BASE       0x3F300000

#define EMMC_ARG2       ((volatile unsigned int*)(EMMC_BASE + 0x00))
#define EMMC_BLKSIZECNT ((volatile unsigned int*)(EMMC_BASE + 0x04))
#define EMMC_ARG1       ((volatile unsigned int*)(EMMC_BASE + 0x08))
#define EMMC_CMDTM      ((volatile unsigned int*)(EMMC_BASE + 0x0C))
#define EMMC_RESP0      ((volatile unsigned int*)(EMMC_BASE + 0x10))
#define EMMC_RESP1      ((volatile unsigned int*)(EMMC_BASE + 0x14))
#define EMMC_RESP2      ((volatile unsigned int*)(EMMC_BASE + 0x18))
#define EMMC_RESP3      ((volatile unsigned int*)(EMMC_BASE + 0x1C))
#define EMMC_DATA       ((volatile unsigned int*)(EMMC_BASE + 0x20))
#define EMMC_STATUS     ((volatile unsigned int*)(EMMC_BASE + 0x24))
#define EMMC_CONTROL0   ((volatile unsigned int*)(EMMC_BASE + 0x28))
#define EMMC_CONTROL1   ((volatile unsigned int*)(EMMC_BASE + 0x2C))
#define EMMC_INTERRUPT  ((volatile unsigned int*)(EMMC_BASE + 0x30))
#define EMMC_IRPT_MASK  ((volatile unsigned int*)(EMMC_BASE + 0x34))
#define EMMC_IRPT_EN    ((volatile unsigned int*)(EMMC_BASE + 0x38))
#define EMMC_CONTROL2   ((volatile unsigned int*)(EMMC_BASE + 0x3C))
#define EMMC_SLOTISR_VER ((volatile unsigned int*)(EMMC_BASE + 0xFC))

// Command flags
#define CMD_NEED_APP        0x80000000
#define CMD_RSPNS_48        0x00020000
#define CMD_ERRORS_MASK     0xfff9c004
#define CMD_RCA_MASK        0xffff0000

// SD card commands
#define CMD_GO_IDLE         0x00000000
#define CMD_ALL_SEND_CID    0x02010000
#define CMD_SEND_REL_ADDR   0x03020000
#define CMD_CARD_SELECT     0x07030000
#define CMD_SEND_IF_COND    0x08020000
#define CMD_SEND_CSD        0x09010000
#define CMD_SEND_CID        0x0A010000
#define CMD_VOLTAGE_SWITCH  0x0B020000
#define CMD_STOP_TRANS      0x0C030000
#define CMD_SEND_STATUS     0x0D020000
#define CMD_SET_BLOCKLEN    0x10020000
#define CMD_READ_SINGLE     0x11220010
#define CMD_READ_MULTI      0x12220032
#define CMD_SET_BLOCKCNT    0x17020000
#define CMD_WRITE_SINGLE    0x18220000
#define CMD_WRITE_MULTI     0x19220032
#define CMD_APP_CMD         0x37000000
#define CMD_SET_BUS_WIDTH   (0x06020000|CMD_NEED_APP)
#define CMD_SEND_OP_COND    (0x29020000|CMD_NEED_APP)
#define CMD_SEND_SCR        (0x33220010|CMD_NEED_APP)

static unsigned int sd_rca = 0;
static unsigned int sd_scr[2];
static int sd_initialized = 0;

extern void delay_cycles(unsigned int count);

static void sd_delay(int count) {
    volatile int i;
    for (i = 0; i < count; i++) {
        asm volatile("nop");
    }
}

static int sd_wait_for_cmd(void) {
    int timeout = 1000000;
    while ((*EMMC_STATUS & 1) && timeout--) {
        sd_delay(100);
    }
    if (timeout <= 0) return SD_ERROR;
    return SD_OK;
}

static int sd_send_cmd(unsigned int cmd, unsigned int arg) {
    // Wait for command line to be ready
    if (sd_wait_for_cmd() != SD_OK) {
        return SD_ERROR;
    }
    
    // Clear interrupt flags
    *EMMC_INTERRUPT = *EMMC_INTERRUPT;
    
    // Send command
    *EMMC_ARG1 = arg;
    *EMMC_CMDTM = cmd;
    
    // Wait for command complete
    int timeout = 1000000;
    while (!(*EMMC_INTERRUPT & 1) && timeout--) {
        sd_delay(1);
    }
    
    if (timeout <= 0) {
        return SD_TIMEOUT;
    }
    
    // Check for errors
    unsigned int resp = *EMMC_INTERRUPT;
    if (resp & CMD_ERRORS_MASK) {
        return SD_ERROR;
    }
    
    return SD_OK;
}

int sd_init(void) {
    uart_puts("Initializing SD card...\n");
    
    // Reset controller
    *EMMC_CONTROL1 = 0;
    *EMMC_CONTROL2 = 0;
    
    // Enable internal clock
    *EMMC_CONTROL1 |= (1 << 0);
    sd_delay(10000);
    
    // Set clock to 400kHz (identification mode)
    *EMMC_CONTROL1 = (0xF9 << 8) | (1 << 0);
    sd_delay(10000);
    
    // Enable SD clock
    *EMMC_CONTROL1 |= (1 << 2);
    sd_delay(10000);
    
    // CMD0: GO_IDLE_STATE
    if (sd_send_cmd(CMD_GO_IDLE, 0) != SD_OK) {
        uart_puts("SD: CMD0 failed\n");
        return SD_ERROR;
    }
    
    // CMD8: SEND_IF_COND (check voltage)
    if (sd_send_cmd(CMD_SEND_IF_COND, 0x1AA) != SD_OK) {
        uart_puts("SD: CMD8 failed\n");
        return SD_ERROR;
    }
    
    // ACMD41: SD_SEND_OP_COND (initialize card)
    int timeout = 1000;
    while (timeout--) {
        if (sd_send_cmd(CMD_APP_CMD, 0) != SD_OK) continue;
        if (sd_send_cmd(CMD_SEND_OP_COND, 0x51FF8000) != SD_OK) continue;
        
        if (*EMMC_RESP0 & 0x80000000) break;
        sd_delay(10000);
    }
    
    if (timeout <= 0) {
        uart_puts("SD: ACMD41 timeout\n");
        return SD_TIMEOUT;
    }
    
    // CMD2: ALL_SEND_CID
    if (sd_send_cmd(CMD_ALL_SEND_CID, 0) != SD_OK) {
        uart_puts("SD: CMD2 failed\n");
        return SD_ERROR;
    }
    
    // CMD3: SEND_RELATIVE_ADDR
    if (sd_send_cmd(CMD_SEND_REL_ADDR, 0) != SD_OK) {
        uart_puts("SD: CMD3 failed\n");
        return SD_ERROR;
    }
    sd_rca = *EMMC_RESP0 & CMD_RCA_MASK;
    
    // CMD7: SELECT_CARD
    if (sd_send_cmd(CMD_CARD_SELECT, sd_rca) != SD_OK) {
        uart_puts("SD: CMD7 failed\n");
        return SD_ERROR;
    }
    
    // Set block size to 512 bytes
    if (sd_send_cmd(CMD_SET_BLOCKLEN, 512) != SD_OK) {
        uart_puts("SD: Set block size failed\n");
        return SD_ERROR;
    }
    
    sd_initialized = 1;
    uart_puts("SD card initialized successfully\n");
    
    return SD_OK;
}

int sd_read_block(unsigned int block, unsigned char* buffer) {
    if (!sd_initialized) return SD_ERROR;
    
    // Set block size and count
    *EMMC_BLKSIZECNT = (1 << 16) | 512;
    
    // Send read command
    if (sd_send_cmd(CMD_READ_SINGLE, block) != SD_OK) {
        return SD_ERROR;
    }
    
    // Read data
    int timeout = 1000000;
    for (int i = 0; i < 128; i++) {
        while (!(*EMMC_STATUS & (1 << 5)) && timeout--) {
            if (timeout <= 0) return SD_TIMEOUT;
        }
        ((unsigned int*)buffer)[i] = *EMMC_DATA;
    }
    
    return SD_OK;
}

int sd_write_block(unsigned int block, const unsigned char* buffer) {
    if (!sd_initialized) return SD_ERROR;
    
    // Set block size and count
    *EMMC_BLKSIZECNT = (1 << 16) | 512;
    
    // Send write command
    if (sd_send_cmd(CMD_WRITE_SINGLE, block) != SD_OK) {
        return SD_ERROR;
    }
    
    // Write data
    int timeout = 1000000;
    for (int i = 0; i < 128; i++) {
        while (!(*EMMC_STATUS & (1 << 4)) && timeout--) {
            if (timeout <= 0) return SD_TIMEOUT;
        }
        *EMMC_DATA = ((unsigned int*)buffer)[i];
    }
    
    return SD_OK;
}
