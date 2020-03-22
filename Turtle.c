//-----------------------------------------------------------------------------
//	Turtle Board Atmel Code
//	Martin Cooper
//	Turtle.c
//-----------------------------------------------------------------------------
/*
             LUFA Library
     Copyright (C) Dean Camera, 2013.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2013  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/
//-----------------------------------------------------------------------------
/*	LOADING CODE

	To use dfu-programmer to program the device:
	hold down white button
	short R119

	run these commands:
		dfu-programmer atmega32u2 erase
		dfu-programmer atmega32u2 flash Turtle.hex
		dfu-programmer atmega32u2 launch

	The commands erase the Atmel, re-program it and then run the new code.

	BOOTLOADER

	To replace the bootloader on an erased Atmel, load DFU_Bootloader.hex using
	a programmer (e.g. AVRISP2) and set fuse BLB1 to SPM_DISABLE

	To make a hex file with the bootloader included, take the ordinary .hex file, delete the last line
	(:00000001FF) and append dfu.hex. Don't forget to set the SPM_DISABLE fuse.

//-----------------------------------------------------------------------------
	TO DO:

	Find out why blank check is so slow and fix it
	See if it's possible to set the MAC address

*/

#include <ctype.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include "Turtle.h"
#include "platform.h"
#include "flash.h"
#include "serialio.h"
#include "USB.h"
#include "sd.h"

/** Circular buffer to hold data from the host before it is sent to the device via the serial port. */
static RingBuffer_t USBtoUSART_Buffer;

/** Underlying data buffer for \ref USBtoUSART_Buffer, where the stored bytes are located. */
static uint8_t      USBtoUSART_Buffer_Data[64];

/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface =
	{
		.Config =
			{
				.ControlInterfaceNumber         = 0,
				.DataINEndpoint                 =
					{
						.Address                = CDC_TX_EPADDR,
						.Size                   = CDC_TXRX_EPSIZE,
						.Banks                  = 1,
					},
				.DataOUTEndpoint                =
					{
						.Address                = CDC_RX_EPADDR,
						.Size                   = CDC_TXRX_EPSIZE,
						.Banks                  = 1,
					},
				.NotificationEndpoint           =
					{
						.Address                = CDC_NOTIFICATION_EPADDR,
						.Size                   = CDC_NOTIFICATION_EPSIZE,
						.Banks                  = 1,
					},
			},
	};

FILE fusb;
FILE *fio;
volatile uint16 gTicks;

//-----------------------------------------------------------------------------
static void ReadComparator(void) {
	
	ACSR = 0b01010000;
	gFlags.powfail = false;
	for (uint8 chan = 2; chan < 5; chan++) {
		if (ACSR & 0x20)
		gFlags.powfail = true;
		ACMUX = chan;
		asm volatile("nop");
		asm volatile("nop");
	}
	
//	if (gFlags.pgmMode)
//		return;
	
	if (gFlags.powfail)
//		POW_GOOD_LO;
	gFlags.ledState = LED_SLOW;
		else
	gFlags.ledState = LED_FAST;
//		POW_GOOD_HI;
}

//-----------------------------------------------------------------------------
//	Called every 100ms
//	In normal mode, just checks the power supplies and the button
//	In pgmMode, does lots more
//-----------------------------------------------------------------------------
ISR(TIMER1_COMPA_vect) {

	static uint8 flasher = 0;
	static uint8 buttCounter = 0;
	
	TIFR1 |= 2;							// clear irq flag
	
	switch (gFlags.buttonState) {
		case BUTT_IDLE:
			if (!MODE_BUTT_PRESS)
				break;
			buttCounter = 0;
			gFlags.buttonState = BUTT_DOWN;
			break;

		case BUTT_DOWN:
			buttCounter++;
			if (!MODE_BUTT_PRESS && (buttCounter < BUTT_LONG)) {
//				if (!gFlags.debounce)
					gFlags.shortPress = true;
//				else
//					gFlags.debounce = false;
				gFlags.buttonState = BUTT_RELEASED;
			}
		
			if (buttCounter > BUTT_LONG) {
				gFlags.longPress = true;
				gFlags.buttonState = BUTT_RELEASED;
			}
			break;

		case BUTT_RELEASED:
			if (!MODE_BUTT_PRESS)					// wait until button released
				gFlags.buttonState = BUTT_IDLE;
			break;
	}
	
	ReadComparator();

	// that's it for normal mode
	if (!gFlags.pgmMode)
		return;
	
	// pgmMode
	if (gTicks) gTicks--;
	if (gSdTimeout) gSdTimeout--;
	if (gSdTimeout2) gSdTimeout2--;
//	if (gSdTimeout3) gSdTimeout3--;

	if (gFlags.pgmMode) {
		switch (gFlags.ledState) {
			case LED_IDLE:
				DEBUG_HI;
				flasher = 0;
				break;
			case LED_SLOW:
				if (++flasher < 8)
					DEBUG_HI;
				else
					DEBUG_LO;
				flasher &= 0x0F;
				break;
			case LED_MED:
				if (++flasher < 4)
					DEBUG_HI;
				else
					DEBUG_LO;
				flasher &= 0x07;
				break;
			case LED_FAST:
				if (++flasher < 2)
					DEBUG_HI;
				else
					DEBUG_LO;
				flasher &= 0x03;
				break;
		}
	}
}

//-----------------------------------------------------------------------------
//	Called when SW1 pressed or released
//-----------------------------------------------------------------------------
ISR(PCINT1_vect) {

	FPGA_RESET;
	Delay_MS(200);
	FPGA_RELEASE;
	
	PCIFR = 2;							// clear interrupt
}

//-----------------------------------------------------------------------------
void InitTimers(uint8 on) {
	
	if (on) {
		// configure timer 1 to give an interrupt for timeouts & delays
		TCCR1A = 0;							// CTC mode
		TCNT1 = 0;							// zero the counter
		TCCR1B = 0x0D;  					// timer0 prescale 1/1024 - CTC Mode @ 7813Hz
		OCR1A = 781;						// Gives interrupts every 100ms at 8 MHz CPU
		TIMSK1 |= (1 << OCIE1A);			// interrupt on match
		} else {
		TCCR1B = 0;							// stop timer
//		TIMSK1 &= ~(1 << OCIE1A);			// disable interrupt on match
	}
	TIFR1 = 0xFF;							// clear all interrupts
	
	gFlags.buttonState = BUTT_IDLE;
	gFlags.shortPress = gFlags.longPress = false;
	//	gFlags.debounce = true;
}

//-----------------------------------------------------------------------------
void SetupHardware(void) {
	
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	DDRB = 0b11110111;
	DDRC = 0;
	DDRD = 0b00001000;
	PORTC |= 0x40;						// enable pull-up on button

	FPGA_RELEASE;
	FLASH_RELEASE;
	SD_RELEASE;
	
	clock_prescale_set(clock_div_1);	// Disable clock division
	InitTimers(true);
	USB_Init();
	SpiInit(false);
	SerialInit(true);
	
	// Init comparator
	ACSR = 0b01010000;
//	DIDR1 = 0b01110000;
	
	// interrupt when SW1 (C6 = INT8) pressed
	PCMSK1 = 1;
	PCICR = 2;
}

//-----------------------------------------------------------------------------
/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void) {
	
	bool ConfigSuccess = true;

	ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
}

//-----------------------------------------------------------------------------
/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void) {
	
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}

//-----------------------------------------------------------------------------
void WaitForBytes(void) {
	
	while (CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface) == 0);
}

//-----------------------------------------------------------------------------
//	Waits until a character is available
//-----------------------------------------------------------------------------
uint8 USBgetch(char *c) {
	
	int16	x;
	
	do {
		HandleUsb();
	} while ((x = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface)) < 0);
	
	*c = (char)(lsb(x));

	return true;
}

//-----------------------------------------------------------------------------
//	Waits until a character is available
//-----------------------------------------------------------------------------
uint8 Getch(char *c) {

	return USBgetch(c);
}

//-----------------------------------------------------------------------------
void PrintHelp(void) {
	
	fputs_P(PSTR("\r\n\r\nTurtle FPGA Programmer\r\n"), fio);
	fputs_P(PSTR("======================\r\n\r\n"), fio);
	fputs_P(PSTR("Commands:\r\n"), fio);
	fputs_P(PSTR("\tB\tverify FPGA configuration erased\r\n"), fio);
	fputs_P(PSTR("\tD\tput into DFU mode for upgrading this firmware\r\n"), fio);
	fputs_P(PSTR("\tE\terase FPGA configuration\r\n"), fio);
	fputs_P(PSTR("\tH\tprint this help message\r\n"), fio);
//	fputs_P(PSTR("\tM\tset MAC address\r\n"), fio);
	fputs_P(PSTR("\tV\tverify FPGA configuration against SD card\r\n"), fio);
	fputs_P(PSTR("\tW\twrite FPGA configuration from SD card\r\n"), fio);
	fputs_P(PSTR("\tX\texit programmer mode and run\r\n"), fio);
}

//-----------------------------------------------------------------------------
//	Short press from run mode gets you to programmer mode, for use with console menu.
//-----------------------------------------------------------------------------
void ProcessCommand(char command) {
	
	gFlags.error = false;
	gFlags.ledState = LED_MED;
	
	switch (toupper(command)) {
		case 'B':
			CheckBlank();
			break;
		
		case 'D':
			fputs_P(PSTR("This will start the firmware upgrade process,\r\npress 'c' to continue, any other key to cancel\r\n"), fio);
			char c;
			Getch(&c);
			fputc(c, fio);
			if (c != 'c') {
				PrintHelp();
				break;
			}
			HandleUsb();

			fputs_P(PSTR("Disconnect your serial connection, rebooting into DFU mode in 8"), fio);
			gFlags.ledState = LED_IDLE;
			DEBUG_HI;
			for (uint8 i = 7; i; i--) {
				for (gSdTimeout = TICK_FREQ; gSdTimeout; )
					HandleUsb();
				fprintf_P(fio, PSTR(".%d"), i);
			}
			EmptyTxBuf();
			cli();
			DEBUG_LO;
			wdt_reset();									// reboot
			wdt_enable(WDTO_30MS);
			for (;;);
			break;

		case 'E':
			EraseFlash();
			break;
		
		case 'H':
			PrintHelp();
			break;

		case 'P':
			ExtReadFlash();
			break;
		
		case 'V':
			CfgVerify();
			break;

		case 'W':
			gFlags.error |=  CfgCopy();
			break;

		case 'X':
			pf_mount(false);
//			InitTimers(false);
			SpiInit(false);															// release the SPI bus
			SerialInit(true);
			Delay_MS(1000);
			fputs_P(PSTR("changing to run mode\r\n"), fio);
			gFlags.pgmMode = false;
			DEBUG_LO;
			break;

		default:
			PrintHelp();
			break;
	}
//	if (gFlags.pgmMode)
//		gFlags.ledState = LED_IDLE;
}

//-----------------------------------------------------------------------------
void HandleUsb(void) {
	
	CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
	USB_USBTask();
}

//-----------------------------------------------------------------------------
//	The UART to the FPGA uses fput(&fuart) etc
//	The USB serial port uses fput(&fusb) etc which I redirected as fio
//	It should be possible to access the USB serial port using stdin / stdout but flaky things happen when you try -
//		puts() and putc() work but not printf
//		Don't mess with it, you will go insane
//-----------------------------------------------------------------------------
int main(void) {
	
/*	// check to see if we need to jump to bootloader
	f_ptr_t booty = (f_ptr_t)0x7000;

	if (MCUSR & 0x08)									// if the reset was caused by watchdog
		booty();										// jump to bootloader
	
	// normal start*/
	int16 rxByte;
	uint16 BufferCount;
	char c;
	
	SetupHardware();

	fio = &fusb;
	
	RingBuffer_InitBuffer(&USBtoUSART_Buffer, USBtoUSART_Buffer_Data, sizeof(USBtoUSART_Buffer_Data));
	CDC_Device_CreateStream(&VirtualSerial_CDC_Interface, &fusb);
	
	gFlags.pgmMode = false;
	GlobalInterruptEnable();
	
	for (;;) {
		// check mode
		if (!gFlags.pgmMode && gFlags.shortPress) {								// enter program mode on button press
			gFlags.shortPress = false;
			gFlags.pgmMode = true;
			fputs_P(PSTR("\r\nChanging to programmer mode\r\n"), fio);
			HandleUsb();
			SerialInit(false);
			//			InitTimers(true);
			SpiInit(true);														// take over the SPI bus
			fputs_P(PSTR("Mounting SD drive\r\n"), fio);
			pf_mount(true);														// mount SD card
			PrintHelp();
			HandleUsb();
			gFlags.ledState = LED_IDLE;
		}
		
		if (gFlags.pgmMode) {
			if (gFlags.shortPress) {											// exit program mode on button press
				gFlags.shortPress = false;
				ProcessCommand('X');
			}

			// check for long button press
			if (gFlags.longPress) {
				gFlags.longPress = false;
			}
		}


		// check incoming characters from USB
		if (!TxBufFull()) {																// look for character if there's room for it in the UART tx buffer
			if ((rxByte = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface)) >= 0) {	// get character if there is one
				if (gFlags.pgmMode)
					ProcessCommand(rxByte);
				else																	// pass-through mode
					UartPutch(rxByte);
			}
		}

		// check characters from UART
		if (!gFlags.pgmMode && (BufferCount = UartChars())) {
			Endpoint_SelectEndpoint(VirtualSerial_CDC_Interface.Config.DataINEndpoint.Address);

			// Check if a packet is already enqueued to the host - if so, we shouldn't try to send more data
			// until it completes as there is a chance nothing is listening and a lengthy timeout could occur 
			if (Endpoint_IsINReady()) {
				// Never send more than one bank size less one byte to the host at a time, so that we don't block
				// while a Zero Length Packet (ZLP) to terminate the transfer is sent if the host isn't listening 
				uint8_t BytesToSend = MIN(BufferCount, (CDC_TXRX_EPSIZE - 1));

				// Read bytes from the USART receive buffer into the USB IN endpoint
				while (BytesToSend--) {
					// Try to send the next byte of data to the host, abort if there is an error without dequeuing
					if (!UartGetch(&c))
						break;
					if (CDC_Device_SendByte(&VirtualSerial_CDC_Interface, c) != ENDPOINT_READYWAIT_NoError)
						break;
				}
			}
		}
		HandleUsb();
	}
}

