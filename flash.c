//-----------------------------------------------------------------------------
//	Turtle Board Atmel Code
//	Martin Cooper
//	flash.c
//	Configuration handler for serial flash
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

	SpiTransferByte(READ_STATUS);		// send Status Read command

	return SpiTransferByte(0);		// get response
}

//-----------------------------------------------------------------------------
//	Waits for WIP bit (bit 0) of STATUS byte to go low
//-----------------------------------------------------------------------------
static void WaitForReady(void) {

	FLASH_SEL;
	while (ReadFlashStatus() & 0x01);
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

	fputs_P(PSTR("Writing configuration:\r\n"), fio);
	if ((res = pf_open("/fpga.bin")) != FR_OK) {
		fprintf_P(fio, PSTR("res = %d\r"), res);
		HandleUsb();
		goto failed;
	}
	
	addr = 0;
	bytesWritten = 0;
	for (addr = 0, done = false; !done; addr += FLASH_PAGE_SIZE) {
		// read page from file
		res = pf_read(buffer, FLASH_PAGE_SIZE * sizeof(uint8), &bytesRead);
		if ((bytesRead != FLASH_PAGE_SIZE * sizeof(int8)) || res)			// end of file or error
			done = true;

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
	if (res)
		fputs_P(PSTR("failed \r\n"), fio);
	else
		fputs_P(PSTR("complete\r\n"), fio);
	return res;
}

//-----------------------------------------------------------------------------
//	Use: call ReadFlash(FLASH_START, addr) to initialise
//		 then repeatedly call ReadFlash(FLASH_CONT, 0) as required
//		 call ReadFlash(FLASH_END, 0) to tidy up when finished
//-----------------------------------------------------------------------------
uint8 ReadFlash(uint8 mode, uint32 address) {

	uint8 data;
	
	cli();
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
	sei();
	return data;
}

//-----------------------------------------------------------------------------
void CfgVerify(void) {

	uint8 b, buffer[FLASH_PAGE_SIZE], done, res;
	uint16 j, bytesRead;
	uint32 addr, errors;
	
	fputs_P(PSTR("Verifying configuration:\r\n"), fio);
	if ((res = pf_open("/fpga.bin")) != FR_OK) {
		fputs_P(PSTR("Failed to read SD card\r\n"), fio);
		return;
	}

	for (addr = 0, done = false, errors = 0; !done; addr += FLASH_PAGE_SIZE) {
		if (!(addr % 10240)) {
			fprintf_P(fio, PSTR("%ld kb\r"), addr >> 10);
			HandleUsb();
		}

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
		fprintf_P(fio, PSTR("failed with %u errors\r\n"), errors);
	else
		fputs_P(PSTR("passed \r\n"), fio);
}

//-----------------------------------------------------------------------------
void CheckBlank(void) {

	uint8 b, failed, aborted, errors;
	uint16 j;
	uint32 addr;
	
	fputs_P(PSTR("Blank check, press any key to abort:\r\n"), fio);
	for (addr = 0, aborted = failed = false, errors = 0; addr <= MAX_FLASH && !failed; addr += FLASH_PAGE_SIZE) {
		if (!(addr % 10240)) {
			fprintf_P(fio, PSTR("%ld kb\r"), addr >> 10);
			HandleUsb();
		}

		// read flash a page at a time, otherwise it's really slow
		ReadFlash(FLASH_START, addr);
		for (j = 0; j < FLASH_PAGE_SIZE; j++) {
			b = ReadFlash(FLASH_CONT, 0);
			if (b != 0xFF) {
				failed = true;
				if (++errors < 30) {
					EmptyTxBuf();
					fprintf_P(fio, PSTR("%08lX: %02X\r\n"), addr, b);
				}
			}
		}
		ReadFlash(FLASH_END, 0);

		if (CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface) >= 0) {	// get character if there is one
			aborted = true;
			break;
		}
	}
	
	if (failed)
		fputs_P(PSTR("failed \r\n"), fio);
	else if (aborted)
		fputs_P(PSTR("aborted \r\n"), fio);
	else
		fputs_P(PSTR("passed \r\n"), fio);
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

	fputs_P(PSTR("Erasing flash (takes < 60s...)"), fio);
	HandleUsb();

//	fprintf_P(fio, PSTR(" SR = %02X\r\n"), ReadFlashStatus());
//	HandleUsb();

	WriteEnable(true);
	FLASH_SEL;
	SpiTransferByte(BULK_ERASE);
	FLASH_DESEL;

	WaitForReady();
	fputs_P(PSTR(" done\r\n"), fio);
	FLASH_SEL;
	FLASH_DESEL;
}

//-----------------------------------------------------------------------------
void ResetFlash(void) {

	FLASH_SEL;
	SpiTransferByte(RSTEN);
	FLASH_DESEL;
	FLASH_SEL;
	SpiTransferByte(RST);
	FLASH_DESEL;
	Delay_MS(20);
}

//-----------------------------------------------------------------------------
void SpiInit(uint8 on) {

	if (on) {
		FPGA_RESET;							// hold FPGA in reset
		FLASH_DESEL;
		FLASH_GRAB;							// make FLASH-CS output
		FLASH_DESEL;
		SD_DESEL;
		SD_GRAB;							// make SD-CS output
		SD_DESEL;
		DDRB |= 0b00000110;					// make SPI lines outputs

		// Setup for SPI Mode 0, MSB first, master mode
		SPSR = 1;							// double speed
		SPCR = 0b01010000;					// f = FCLK / 2, mode 0
//		SPCR = 0b01010001;					// f = FCLK / 8, mode 0
//		SPCR = 0b01010010;					// f = FCLK / 32, mode 0
		ResetFlash();						// now reset the flash
	} else {
		FLASH_DESEL;
		SD_DESEL;
		SPCR = 0;							// disable SPI
		DDRB &= ~0b00000110;				// release SPI lines
		FLASH_RELEASE;						// release FLASH-CS
		SD_RELEASE;							// release SD-CS
		FPGA_RELEASE;
	}
}

