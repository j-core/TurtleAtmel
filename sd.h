//-----------------------------------------------------------------------------
//	Turtle Board Atmel Code
//	Martin Cooper
//	sd.h
//-----------------------------------------------------------------------------
#ifndef SD_H_
#define SD_H_

#include "Turtle.h"
//#include "integer.h"
#include "pff.h"

typedef uint8	DSTATUS;


// Results of Disk Functions
typedef enum {
	RES_OK = 0,		// 0: Function succeeded
	RES_ERROR,		// 1: Disk error
	RES_NOTRDY,		// 2: Not ready
	RES_PARERR		// 3: Invalid parameter
} DRESULT;

#define STA_NOINIT		0x01	/* Drive not initialized */
#define STA_NODISK		0x02	/* No medium in the drive */

#define SD_BLOCK_SIZE 		512
//#define SD_TIMEOUT			TICK_FREQ / 2
#define SD_TIMEOUT			2				// * 100ms

/* Card type flags (sCardType) */
#define CT_MMC				0x01	/* MMC ver 3 */
#define CT_SD1				0x02	/* SD ver 1 */
#define CT_SD2				0x04	/* SD ver 2 */
#define CT_SDC				(CT_SD1|CT_SD2)	/* SD */
#define CT_BLOCK			0x08	/* Block addressing */

#define NON_ZERO			0xFF
#define RECORD_TOKEN		0xAA

//#define SLOW				2

#define CMD0				0				// GO_IDLE_STATE
#define CMD1				1				// SEND_OP_COND (MMC)
#define ACMD41				0x80 + 41		// SEND_OP_CMD (SDC)
#define CMD8				8				// SEND_IF_CMD
#define CMD9				9				// SEND_CSD
#define CMD10				10				// SEND_CID
#define CMD12				12				// STOP_TRANSMISSION
#define CMD13				13				// SEND_STATUS
#define CMD16				16				// SET_BLOCKLEN
#define CMD17				17				// READ_SINGLE_BLOCK
#define CMD23				23				// SET_BLOCK_COUNT
#define CMD24				24				// WRITE_SINGLE_BLOCK
#define CMD25				25				// WRITE_MULTIPLE_BLOCK
#define CMD32				32				// ERASE_BLOCK_START
#define CMD33				33				// ERASE_BLOCK_END
#define CMD38				38				// ERASE_BLOCKS
#define CMD55				55				// APP_CMD
#define CMD58				58				// READ_OCR

extern volatile	uint8	gSdTimeout, gSdTimeout2;
//extern			uint8 	gSdBlock[SD_BLOCK_SIZE];

DRESULT disk_readp (uint8 *, uint32, uint16, uint16);
DSTATUS disk_initialize(void);
//DRESULT disk_writep (const BYTE*, DWORD);
//void	ReadDir(void);
//FRESULT ReadLine(char *p, int maxLen);

#endif
