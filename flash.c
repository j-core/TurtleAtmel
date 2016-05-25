//-----------------------------------------------------------------------------
//	Turtle Board Atmel Code
//	Martin Cooper
//	flash.c
//	Configuration flash handler for M25P16 serial flash
//	page size = 256 bytes
//-----------------------------------------------------------------------------
#include <avr/interrupt.h>
#include <avr/io.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>

#include "platform.h"
#include "flash.h"
#include "serialio.h"
#include "Turtle.h"
#include "pff.h"

//-----------------------------------------------------------------------------
uint8 SpiTransferByte(uint8 send) {

	SPDR = send;
	
	while (!(SPSR & 0x80));
	
	return SPDR;
}

//-----------------------------------------------------------------------------
static uint8 ReadFlashStatus(void) {

	uint8 status;
	
	SpiTransferByte(READ_STATUS);		// send Status Read command
	status = SpiTransferByte(0);		// get response
	
	return status;
}

//-----------------------------------------------------------------------------
//	Waits for WIP bit (bit 0) of STATUS byte to go low
//-----------------------------------------------------------------------------
static void WaitForReady(void) {

	FLASH_SEL;
//	while (ReadFlashStatus() & 0x01);
	uint8 s;
	do {
		s = ReadFlashStatus();
	} while (s & 0x01);
	FLASH_DESEL;
}

//-----------------------------------------------------------------------------
static void WriteEnable(uint8 we) {

	FLASH_SEL;
	if (we)
		SpiTransferByte(WRITE_ENABLE);
	else
		SpiTransferByte(WRITE_DISABLE);
	FLASH_DESEL;
}

//-----------------------------------------------------------------------------
//	Use: call WriteFlash(FLASH_START, addr, 0) to get going
//		 then repeatedly call WriteFlash(FLASH_CONT, 0, data) as required
//		 call WriteFlash(FLASH_END, 0, 0) to tidy up when finished
//-----------------------------------------------------------------------------
void WriteFlash(uint8 mode, uint32 address, uint8 data) {

	static uint32 addr;
	static uint8 writing = false;
	
	if (writing) {
		WaitForReady();
		writing = false;

		WriteEnable(true);									// start writing next page
		FLASH_SEL;
		SpiTransferByte(PAGE_PROGRAM);
		SpiTransferByte(addr >> 16);
		SpiTransferByte(addr >> 8);
		SpiTransferByte(addr);
	}
	
	switch (mode) {
		case FLASH_START: 
			addr = address;
			WriteEnable(true);
			FLASH_SEL;
			SpiTransferByte(PAGE_PROGRAM);
			SpiTransferByte(addr >> 16);
			SpiTransferByte(addr >> 8);
			SpiTransferByte(addr);
			break;

		case FLASH_CONT:
			SpiTransferByte(data);							// write the byte
			addr++;

			// check if next write will cross a page boundary
			if ((addr & 0x000000FF) == 0l)	{
				FLASH_DESEL;								// write the page - takes 5ms
				writing = true;
			}	
			break;

		case FLASH_END:
			FLASH_DESEL;									// write the page
			WaitForReady();									// and wait
			break;

		default:
			break;
	}
}

//-----------------------------------------------------------------------------
uint8 CfgCopy(void) {

	uint8 buffer[FLASH_PAGE_SIZE], res, done;
	uint16 bytesRead;
	uint32 bytesWritten, addr;
	
	if ((res = pf_open("/fpga.bin")) != FR_OK)
		goto failed;
	
	addr = 0;
	bytesWritten = 0;
	for (addr = 0, done = false; !done; addr += FLASH_PAGE_SIZE) {
		// read page from file
		res = pf_read(buffer, FLASH_PAGE_SIZE * sizeof(uint8), &bytesRead);
		if ((bytesRead != FLASH_PAGE_SIZE * sizeof(int8)) || res)			// end of file or error
			done = true;

		SD_DESEL;

		// now write page to flash
		WaitForReady();										// wait for last write to complete

		WriteEnable(true);
		FLASH_SEL;
		SpiTransferByte(PAGE_PROGRAM);
		SpiTransferByte(addr >> 16);
		SpiTransferByte(addr >> 8);
		SpiTransferByte(addr);

		for (uint16 i = 0; i < bytesRead; i++)
			SpiTransferByte(buffer[i]);
		FLASH_DESEL;										// write the page - takes 5ms

		bytesWritten += bytesRead;
		if (!(bytesWritten % 10240))
			fprintf_P(fio, PSTR("%ld kb\r"), bytesWritten / 1024);
		HandleUsb();
	}
	
failed:
	fputs_P(PSTR("\nFLASH programming "), fio);
	if (res)
		fputs_P(PSTR("failed\r\n"), fio);
	else
		fputs_P(PSTR("complete\r\n"), fio);
	return res;
}

//-----------------------------------------------------------------------------
/*void CfgTest(void) {

//	uint8 buffer[FLASH_PAGE_SIZE], res;
//	uint16 bytesRead;
	uint32 addr;
	char c;
	
	USBgetch(&c);
	addr = (uint32)(c - '0') << 8;
			
		SD_DESEL;

		// now write page to flash
		WaitForReady();										// wait for last write to complete

		WriteEnable(true);
		FLASH_SEL;
		SpiTransferByte(PAGE_PROGRAM);
		SpiTransferByte(addr >> 16);
		SpiTransferByte(addr >> 8);
		SpiTransferByte(addr);

		for (uint16 i = 0; i < FLASH_PAGE_SIZE; i++)
			SpiTransferByte((uint8)i);
		FLASH_DESEL;										// write the page - takes 5ms

	fputs_P(PSTR("\nFLASH programming complete \r\n"), fio);
}*/

//-----------------------------------------------------------------------------
//	Use: call ReadFlash(FLASH_START, addr) to initialise
//		 then repeatedly call ReadFlash(FLASH_CONT, 0) as required
//		 call ReadFlash(FLASH_END, 0) to tidy up when finished
//-----------------------------------------------------------------------------
uint8 ReadFlash(uint8 mode, uint32 address) {

	uint8 data;
	
//	cli();
	switch (mode) {
		case FLASH_START:
			FLASH_SEL;
			SpiTransferByte(READ_DATA);
			SpiTransferByte(address >> 16);
			SpiTransferByte(address >> 8);
			SpiTransferByte(address);
			data = 0;
			break;

		case FLASH_CONT:
			data = SpiTransferByte(0);
			break;

		case FLASH_END:
			data = SpiTransferByte(0);
			FLASH_DESEL;
		break;

		default:
			data = 0;
			break;
	}
//	sei();
	return data;
}

//-----------------------------------------------------------------------------
void CfgVerify(void) {

	uint8 b, buffer[FLASH_PAGE_SIZE], done, res;
	uint16 j, bytesRead;
	uint32 addr, errors;
	
	if ((res = pf_open("/fpga.bin")) != FR_OK) {
		fputs_P(PSTR("Failed to read SD card\r\n"), fio);
		return;
	}

	for (addr = 0, done = false, errors = 0; !done; addr += FLASH_PAGE_SIZE) {
		if (!(addr % 10240))
			fprintf_P(fio, PSTR("%ld kb\r"), addr >> 10);
		HandleUsb();

		// read page from file
		res = pf_read(buffer, FLASH_PAGE_SIZE * sizeof(uint8), &bytesRead);
		if ((bytesRead != FLASH_PAGE_SIZE * sizeof(int8)) || res)			// end of file or error
			done = true;

		SD_DESEL;

		// read from flash and compare with buffer[]
		ReadFlash(FLASH_START, addr);
		for (j = 0; j < bytesRead; j++) {
			b = ReadFlash(FLASH_CONT, 0);
			if (b != buffer[j]) {
				if (++errors < 30) {
					EmptyTxBuf();
					fprintf_P(fio, PSTR("%08lX: SD=%02X cfg=%02X\r\n"), addr + j, buffer[j], b);
				}
			}
		}
		ReadFlash(FLASH_END, 0);
	}
	
	if (errors)
		fprintf_P(fio, PSTR("Verify failed with %u errors\r\n"), errors);
	else
		fputs_P(PSTR("Verify passed\r\n"), fio);
}

//-----------------------------------------------------------------------------
// This is ridiculously slow, don't know why
//-----------------------------------------------------------------------------
void CheckBlank(void) {

	uint8 b, failed;
	uint32 addr;
	
	ReadFlash(FLASH_START, 0);
	for (addr = 0, failed = false; addr <= MAX_FLASH; addr++) {
		if (!(addr % 10240))
			fprintf_P(fio, PSTR("%ld kb\r"), addr >> 10);
		HandleUsb();

		// read flash
		b = ReadFlash(FLASH_CONT, 0);
		if (b != 0xFF) {
			failed = true;
			fprintf_P(fio, PSTR("%08lX: %02X\r\n"), addr, b);
			break;
		}
	}
	ReadFlash(FLASH_END, 0);
	
	fputs_P(PSTR("Blank check "), fio);
	if (failed)
		fputs_P(PSTR("failed\r\n"), fio);
	else
		fputs_P(PSTR("passed\r\n"), fio);
}

//-----------------------------------------------------------------------------
void ExtReadFlash(void) {
	
	uint8 b;
	uint8 j;
	char string[19];
	uint32 addr;
	
	// read page number (single digit only)
	USBgetch(string);
	addr = (uint32)(*string - '0') << 8;

	string[16] = '\r';
	string[17] = '\n';
	string[18] = '\0';

	ReadFlash(FLASH_START, addr);
	
	fprintf_P(fio, PSTR("%08lX:\r\n"), addr);
	for (uint16 i = 0; i < FLASH_PAGE_SIZE >> 4; i++) {
		fprintf_P(fio, PSTR("%04X: "), i << 4);
		for (j = 0; j < 16; j++) {
			b = ReadFlash(FLASH_CONT, 0);
			fprintf_P(fio, PSTR("%02X "), b);
			string[j] = isprint(b) ? b : '.';
		}
		fputc('\t', fio);
		fputs(string, fio);
	}
	ReadFlash(FLASH_END, 0);
}

//-----------------------------------------------------------------------------
void EraseFlash(void) {

	fputs_P(PSTR("Erasing flash... "), fio);
	HandleUsb();

	WriteEnable(true);
	FLASH_SEL;
	SpiTransferByte(BULK_ERASE);			// takes 20s
	FLASH_DESEL;

	WaitForReady();
	fputs_P(PSTR("done\r\n"), fio);
	FLASH_SEL;
	FLASH_DESEL;
}

//-----------------------------------------------------------------------------
void SpiInit(uint8 on) {

	if (on) {
		FPGA_RESET;							// hold FPGA in reset
		DDRB |= 0b00000110;					// make SPI lines outputs
		FLASH_GRAB;							// make FLASH-CS output
		FLASH_DESEL;
		SD_GRAB;							// make SD-CS output
		SD_DESEL;
		// Setup for SPI Mode 0, MSB first, master mode
		SPSR = 1;						// double speed
		SPCR = 0b01010000;				// f = FCLK / 2, mode 0
//		SPCR = 0b01010001;				// f = FCLK / 8, mode 0
//		SPCR = 0b01010010;				// f = FCLK / 32, mode 0
	} else {
		SPCR = 0;							// disable SPI
		DDRB &= ~0b00000110;				// release SPI lines
		SD_RELEASE;							// release SD-CS
		FLASH_RELEASE;						// release FLASH-CS
		FPGA_RELEASE;
	}
}

