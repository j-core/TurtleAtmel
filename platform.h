//-----------------------------------------------------------------------------
//	Turtle Board Atmel Code
//	Martin Cooper
//	platform.h
//
//-----------------------------------------------------------------------------
#ifndef PLATFORM_H_
#define PLATFORM_H_

#define uint8	uint8_t
#define int8	int8_t
#define uint16	uint16_t
#define int16	int16_t
#define uint32	uint32_t
#define int32	int32_t
#define uint64	uint64_t
#define true	1
#define false	0
#define msb(x) 	(uint8)((uint16)x >> 8)
#define lsb(x)	(uint8)((uint16)x & 0xFF)

#define PCB_1V0				0
#define PCB_1V1				1

#define PCB					PCB_1V1

//#if (PCB == PCB_1V0)
#define FLASH_SEL			asm volatile("cbi 0x05, 7")		// B7
#define FLASH_DESEL			asm volatile("sbi 0x05, 7")		// B7
#define FLASH_RELEASE		asm volatile("cbi 0x04, 7")		// DDRB7
#define FLASH_GRAB			asm volatile("sbi 0x04, 7")		// DDRB7
#define SD_SEL				asm volatile("cbi 0x05, 6")		// B6
#define SD_DESEL			asm volatile("sbi 0x05, 6")		// B6
#define SD_RELEASE			asm volatile("cbi 0x04, 6")		// DDRB6
#define SD_GRAB				asm volatile("sbi 0x04, 6")		// DDRB6
#define FPGA_RELEASE		asm volatile("sbi 0x05, 5")		// B5
#define FPGA_RESET			asm volatile("cbi 0x05, 5")		// B5
#define POW_GOOD_HI			asm volatile("sbi 0x05, 4")		// B4
#define POW_GOOD_LO			asm volatile("cbi 0x05, 4")		// B4

#define DEBUG_HI			POW_GOOD_HI
#define DEBUG_LO			POW_GOOD_LO

#define RQ_FPGA_RESET		(!(PINC & 0x40))
#define MODE_BUTT_PRESS		(!(PIND & 0x80))
//#endif

#define ACMUX _SFR_IO8(0x7D)				// missing from <avr/iom32u2.h>

#define NOP					asm volatile("nop")
#endif