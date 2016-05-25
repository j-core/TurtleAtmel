//-----------------------------------------------------------------------------
//	Turtle Board Atmel Code
//	Martin Cooper
//	sd.c
//-----------------------------------------------------------------------------
//	The SD interface is big-endian but the FAT32 tables etc are little-endian
//-----------------------------------------------------------------------------
//	http://elm-chan.org/docs/mmc/mmc_e.html
//-----------------------------------------------------------------------------
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include "platform.h"
#include "serialio.h"
#include "sd.h"
#include "pff.h"
#include "flash.h"

volatile	uint8	gSdTimeout, gSdTimeout2;
static		uint8	sCardType;

//-----------------------------------------------------------------------------
static uint8 SpiTransfer(uint8 data) {
	
	SPDR = data;
//	while (!(SPSR & 0x80));
	for (gSdTimeout = SD_TIMEOUT; gSdTimeout && !(SPSR & 0x80); );
//	if (!gSdTimeout)
//		fputc('a', fio);
	data = SPDR;
	
	return data;
}

//-----------------------------------------------------------------------------
static void SdDeselect(void) {

	SD_DESEL;
	SpiTransfer(0xFF);
}

//-----------------------------------------------------------------------------
static uint8 SdSelect(void) {

	SD_SEL;
	SpiTransfer(0xFF);

//	while (SpiTransfer(0xFF) != 0xFF);
	for (gSdTimeout2 = SD_TIMEOUT; !gSdTimeout2 && (SpiTransfer(0xFF) != 0xFF); );
/*	if (!gSdTimeout) {
		fputc('b', fio);
		return false;
	} else
		return true;*/
	return gSdTimeout2;
}

//-----------------------------------------------------------------------------
static uint8 SendCommand(uint8 command, uint32 arg) {

	uint8 ret = 0;

	if (command & 0x80) {					// ACMD<n> is the command sequence of CMD55-CMD<n>
		command &= 0x7F;
		if ((ret = SendCommand(CMD55, 0)) > 1) {
			fputs_P(PSTR("err1\r\n"), fio);
			return ret;
		}
	}

	SdDeselect();
	if (!SdSelect()) {
		fputs_P(PSTR("err2\r\n"), fio);
		return 0xFF;
	}

	SpiTransfer(command | 0x40);
	SpiTransfer((uint8)(arg >> 24));
	SpiTransfer((uint8)(arg >> 16));
	SpiTransfer((uint8)(arg >> 8));
	SpiTransfer((uint8)arg);
	switch (command) {						// checksum - only needed for certain commands
		case CMD0:
			SpiTransfer(0x95);
			break;
		case CMD8:
			SpiTransfer(0x87);
			break;
		default:
			SpiTransfer(0xFF);
			break;
	}

	// wait for response
	gSdTimeout2 = TICK_FREQ * 2;
	while (gSdTimeout2) {
		if (!((ret = SpiTransfer(0xFF)) & 0x80))
			return ret;
	}
//	while ((ret = SpiTransfer(0xFF)) & 0x80);
/*	for (uint16 i = 0; i < 10000; i++) {
		if (!((ret = SpiTransfer(0xFF)) & 0x80))
			return ret;
	}
	fputc('c', fio);*/
	return ret;
}

//-----------------------------------------------------------------------------
//	Powers up the disc and initialises it into SPI mode
//-----------------------------------------------------------------------------
DSTATUS disk_initialize(void) {

	uint8 ret = STA_NOINIT;
	uint8 spi;
	
	// Init the card in SPI mode by sending clks for 2 ms @ 250 kHz
	spi = SPCR;								// store previous setting
	SPCR = 0b01010010;						// f = FCLK / 32 = 250kHz, mode 0

	SD_DESEL;
	for (uint16 i = 0; i < 200; i++)
		SpiTransfer(0xFF);
	
	SD_SEL;
	
	SPCR = spi;								// restore SPI speed

	// Send CMD0 GO_IDLE_STATE
	gSdTimeout = SD_TIMEOUT;
	if (SendCommand(CMD0, 0) != 1) {
		if (SendCommand(CMD0, 0) != 1) {
			fputs_P(PSTR("CMD0 failed \r\n"), fio);
			goto SdPowerExit;
		}
	}

	// Send CMD8 to find out what disc type we have
	uint8 ocr[4];
	gSdTimeout = SD_TIMEOUT;
	if ((ocr[0] = SendCommand(CMD8, 0x000001AA)) == 1) {				// this is an SDv2 card
		for (uint8 i = 0; i < 4; i++)						// get the response
			ocr[i] = SpiTransfer(0xFF);
		if (!((ocr[2] == 0x01) && (ocr[3] == 0xAA)))		// check the response
			goto SdPowerExit;								// cannot operate at 2.7 - 3.6V
		fputs_P(PSTR("SDv2 card detected\r\n"), fio);

		// Send ACMD41 SEND_OP_COND until response is 0
		gSdTimeout2 = 2 * TICK_FREQ;
		while (SendCommand(ACMD41, 0x40000000)) {
			if (!gSdTimeout2) {
				fputc('d', fio);
				goto SdPowerExit;
			}
		}

		// now send ACMD58 READ_OCR
		if (SendCommand(CMD58, 0) != 0)
			goto SdPowerExit;

		for (uint8 i = 0; i < 4; i++)						// get the response
			ocr[i] = SpiTransfer(0xFF);
		sCardType = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;	// SDv2 (HC or SC)
	} else {
		uint8 cmd;
		if (SendCommand(ACMD41, 0) <= 1) {					// this is an SDv1 card
			cmd = ACMD41;
			sCardType = CT_SD1;
			fputs_P(PSTR("SDv1 card detected\r\n"), fio);
		} else {											// this is an MMCv3 card
			cmd = CMD1;
			sCardType = CT_MMC;
			fputs_P(PSTR("MMCv3 card detected\r\n"), fio);
		}

		// wait until initialisation finishes
		gSdTimeout2 = TICK_FREQ * 2;
		while (SendCommand(cmd, 0)) {
			if (!gSdTimeout2) {
//				fputc('e', fio);
				goto SdPowerExit;
			}
		}

		// set block size
/*		uint8 resp;
		if ((resp = SendCommand(CMD16, SD_BLOCK_SIZE)) != 0) {
			printf_P(PSTR"CMD16 failed: %02X\r\n", resp);
			goto SdPowerExit;
		}*/
	}

	// everything hunky dory
	ret = 0;

SdPowerExit:
	if (ret) {
		fputs_P(PSTR("SD error\r\n"), fio);
	} 
	SD_DESEL;

	return ret;
}

//-----------------------------------------------------------------------------
//	Reads cnt bytes from SD card block, starting at byte offset offs
//-----------------------------------------------------------------------------
DRESULT disk_readp(uint8 *buff, uint32 blockNum, uint16 offs, uint16 cnt) {

	DRESULT res = RES_ERROR;
	uint8 r, *p;
		
//	printf_P(PSTR("disk_readp(,0x%X, %d, %d)\r\n"), (int)blockNum, offs, cnt);
	
	if (!(sCardType & CT_BLOCK))
		blockNum <<= 9;							// multiply by SD_BLOCK_SIZE

	// read the block from the card
	if ((r = SendCommand(CMD17, blockNum)) != 0) {
		fprintf_P(fio, PSTR("\tCMD17(%X) returned %d\r\n"), blockNum, r);
		goto ReadBlockExit;
	}

	// wait for data token
	gSdTimeout2 = TICK_FREQ / 4;
	while (SpiTransfer(0xFF) != 0xFE) {
		if (!gSdTimeout2) {
//			fputc('f', fio);
			goto ReadBlockExit;
		}
	}

	// read the block
	p = buff;
	uint16 i;
	for (i = 0; i < offs; i++)
		SpiTransfer(0xFF);
	for ( ; i < offs + cnt; i++)
		*p++ = SpiTransfer(0xFF);
	for (; i < SD_BLOCK_SIZE; i++)
		SpiTransfer(0xFF);

	res = RES_OK;

ReadBlockExit:
	SdDeselect();
//	printf_P(PSTR"Read block returned %d\r\n", res);
	
	return res;
}

//-----------------------------------------------------------------------------
FRESULT ReadLine(char *p, uint16 maxLen) {
	
	uint16 bytesRead;
	FRESULT res;
	uint16 len = 0;
	char c;

	maxLen--;
	do {
		res = pf_read(&c, 1, &bytesRead);
		if (res != FR_OK) {
			return res;
		}
		if (bytesRead != 1) {
			return FR_EOF;
		}
		if (++len < maxLen) {
			*p++ = c;
		}
	} while (c != '\n');
	*p = '\0';
	
	return FR_OK;
}

//-----------------------------------------------------------------------------
/*static uint8 ChooseFile(uint32 *size) {
	
	uint8 res;
	DIR dir;
	int i, nFiles;
	FILINFO fno;		// File information
	char s[25];
	
	res = pf_opendir(&dir, "FPGA");
	if (res) {
		fprintf_P(fio, PSTR("opendir() returned %d\r\n"), res);
		return res;
	}
	
	nFiles = 1;
	for (;;) {
		res = pf_readdir(&dir, &fno);
		if (res != FR_OK) {
			fprintf_P(fio, PSTR("readdir() returned %d\r\n"), res);
			break;
		}

		if (!fno.fname[0])				// end of directory
			break;

		if (fno.fattrib & AM_DIR) {
			fprintf_P(fio, PSTR("\t   <DIR>   %s\r\n"), fno.fname);
		} else {
			fprintf_P(fio, PSTR("\t%d\t%s\t%9lu\r\n"), nFiles, fno.fname, fno.fsize);
		}
		nFiles++;
	}
	nFiles--;
	fprintf_P(fio, PSTR("%d item(s)\r\n"), nFiles);

	// ask which file to load
	fputs_P(PSTR("Select file number (0 to abort)\r\n"), fio);
	for (i = 0; i < 5; i++) {
		UartGetch(&s[i]);
		if ((s[i] == '\r') || (s[i] == '\n')) {
			s[i] = '\0';
			break;
		}
	}
	i = atoi(s);
	if (i > nFiles) {
		fputs_P(PSTR("Invalid file number\r\n"), fio);
		return FR_EOF;
	}
	
	// open file
	fprintf_P(fio, PSTR("Opening file #%d\r\n"), i);
	res = pf_opendir(&dir, "FPGA");
	for (int j = 1; j < i; j++)
		res = pf_readdir(&dir, &fno);
		
	strcpy_P(s, PSTR("/FPGA/"));
	strcat(s, fno.fname);;
	if ((res = pf_open(s)) != FR_OK) {
		fprintf_P(fio, PSTR("Unable to open file #%d:%s\tpf_open() returned %d\r\n"), i, s, res);
		return res;
	}
	
	*size = fno.fsize;

	return res;
}*/

