//-----------------------------------------------------------------------------
//	Turtle Board Atmel Code
//	Martin Cooper
//	flash.h
//-----------------------------------------------------------------------------
#ifndef FLASH_H_
#define FLASH_H_

#include "platform.h"

//uint8 	SpiTransferByte(uint8 send);
void	WriteFlash(uint8 mode, uint32 address, uint8 data);
uint8	ReadFlash(uint8 mode, uint32 address);
void 	EraseFlash(void);
void	SpiInit(uint8 on);
void	ExtReadFlash(void);
uint8	CfgCopy(void);
//void	CfgTest(void);
void	CfgVerify(void);
void	CheckBlank(void);
void	ResetFlash(void);

#if PCB == PCB1V0
	#define MAX_FLASH				0x001FFFFFul
#else
	#define MAX_FLASH				0x003FFFFFul
#endif
#define FLASH_PAGE_SIZE			256
#define DEFAULT_FLASH_TIMEOUT	40			// ms
#define ERASE_FLASH_TIMEOUT		12000		// ms

//	Flash commands
#define WRITE_ENABLE			0x06
#define WRITE_DISABLE			0x04
#define READ_ID					0x9F
#define READ_STATUS				0x05
#define WRITE_STATUS			0x01
#define	READ_DATA				0x03
#define READ_DATA_FAST			0x0B
#define PAGE_PROGRAM			0x02
#define SECTOR_ERASE			0xD8
#define BULK_ERASE				0xC7
#define POWER_DOWN				0xB9
#define POWER_UP				0xAB
#define RSTEN					0x66
#define RST						0x99

// write modes - used by & ReadFlash()
#define FLASH_START		0x01
#define FLASH_CONT		0x02
#define FLASH_END		0x03

#endif

