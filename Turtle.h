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

#ifndef TURTLE_H
#define TURTLE_H

/* Includes: */
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <RingBuffer.h>
#include <USB.h>
#include "Descriptors.h"
#include "platform.h"

#define TICK_FREQ			10

extern volatile uint16 gTicks;
struct {
	volatile uint8	tick			: 1;
	volatile uint8	buttonState		: 2;
	volatile uint8	ledState		: 2;
	volatile uint8	shortPress		: 1;
	volatile uint8	longPress		: 1;
	volatile uint8	powfail			: 1;
	uint8			pgmMode			: 1;
	uint8			sdOk			: 1;
	uint8			error			: 1;
	uint8			debounce		: 1;
} gFlags;

// LED states
#define LED_IDLE		0
#define LED_SLOW		1
#define LED_MED			2
#define LED_FAST		3

// Button states
#define BUTT_IDLE		0
#define BUTT_DOWN		1
#define BUTT_RELEASED	2

#define BUTT_LONG		12					// length of a long button press

//#define E2_DFU			(uint8 *)10
//#define DFU				0x69

extern	FILE fusb;
extern	FILE *fio;
extern USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface;
typedef void (*f_ptr_t)(void);

void SetupHardware(void);

void EVENT_USB_Device_ConfigurationChanged(void);
void EVENT_USB_Device_ControlRequest(void);

void EVENT_CDC_Device_LineEncodingChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo);

uint8_t USBgetch(char *c);
uint8_t USBputch(char c);
uint8_t Getch(char *c);
void	HandleUsb(void);
void	USBputs_P(PGM_P p);
void	WaitForBytes(void);
		


#endif

