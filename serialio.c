//-----------------------------------------------------------------------------
//	Turtle Board Atmel Code
//	Martin Cooper
//	serialio.c
//-----------------------------------------------------------------------------
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>
#include <string.h>

#include "platform.h"
#include "serialio.h"

volatile uint16		gGetchTimeout;
static BUFFER 		sRxBuf;
static BUFFER 		sTxBuf;
static uint8 		sRxError;
static uint8 		sRxC;
static uint8		sHandleBackspace;


FILE fuart = FDEV_SETUP_STREAM(UartPut, UartGet, _FDEV_SETUP_RW);

//-----------------------------------------------------------------------------
void FlushSerialRx(void) {
	
	uint8 c;

	UCSR1B &= ~(1 << RXCIE1);				// disable the Rx interrupt
	UCSR1A |= 0x40;							// clear interrupt
	c = UDR1;								// clear error flags
	UCSR1B |= (1 << RXCIE1);				// enable the Rx interrupt

	sRxBuf.count = 0;
	sRxBuf.nextIn = sRxBuf.nextOut = sRxBuf.buffer;
}

//-----------------------------------------------------------------------------
//	Returns true when the last byte in the buffer has been transmitted,
//	as opposed to when the buffer is emptied
//-----------------------------------------------------------------------------
void EmptyTxBuf(void) {

	while (sTxBuf.busy);
}

//-----------------------------------------------------------------------------
uint8 TxBufFull(void) {

	return (sTxBuf.count >= BUFFER_SIZE - 1);
}

//-----------------------------------------------------------------------------
//	uart1 (host) transmit complete isr - executed when USART shift register is empty
//-----------------------------------------------------------------------------
ISR(USART1_TX_vect) {

	UCSR1B &= ~(1 << TXCIE1);					// disable this interrupt
	UCSR1A |= (1 << TXC1);						// clear the tx complete flag
	sTxBuf.busy = false;
}	

//-----------------------------------------------------------------------------
//	uart1 (host) transmit isr - executed when USART Data Register is empty
//-----------------------------------------------------------------------------
ISR(USART1_UDRE_vect) {

//	DEBUG_HI;
	if (!sTxBuf.count) {						// no more characters to send
		UCSR1B &= ~(1 << UDRIE1);				// disable this interrupt
		UCSR1B |= (1 << TXCIE1);				// enable the transmit complete interrupt
		DEBUG_LO;
		return;
	}

	UDR1 = *sTxBuf.nextOut;						// send next character and clear interrupt
	if (++sTxBuf.nextOut == sTxBuf.buffer + BUFFER_SIZE)
		sTxBuf.nextOut = sTxBuf.buffer;			// wraparound

	sTxBuf.count--;
//	DEBUG_LO;
}	

//-----------------------------------------------------------------------------
// adds character to the output buffer unless buffer is full
//-----------------------------------------------------------------------------
void UartPutch(char c) {

	if (TxBufFull())
		return;									// reject if buffer full

	sTxBuf.busy = true;

	// disable tx interrupt
	UCSR1B &= ~(1 << UDRIE1);					
		
	if (sTxBuf.count < BUFFER_SIZE) {			// if buffer not full
		*sTxBuf.nextIn = c;						// load into buffer
		if (++sTxBuf.nextIn == sTxBuf.buffer + BUFFER_SIZE) 
			sTxBuf.nextIn = sTxBuf.buffer;		// wraparound
		sTxBuf.count++;

		if (sTxBuf.count > sTxBuf.maxCount)
			sTxBuf.maxCount = sTxBuf.count;
	}
		
	// enable tx interrupt
	UCSR1B |= 1 << UDRIE1;						
}

//-----------------------------------------------------------------------------
// Writes a string to the UART
//-----------------------------------------------------------------------------
void UartPuts(char *c) {

	while(*c)
		UartPutch(*c++);
}

//-----------------------------------------------------------------------------
//	Wrapper function to allow use of fprintf() for IO
//-----------------------------------------------------------------------------
int UartPut(char c, FILE *stream) {
	
	while (sTxBuf.count >= BUFFER_SIZE - 1);

	UartPutch((char)c);
	
	return 0;
}

//-----------------------------------------------------------------------------
//	uart1 (host) receive isr
//	Adds incoming character to the circular buffer
//	overwrites oldest if buffer is full
//-----------------------------------------------------------------------------
ISR(USART1_RX_vect) {

//	DEBUG_HI;
	sRxError = UCSR1A;							// read the error flags
	sRxC = UDR1;								// get character & clear interrupt

	if ((sRxError & (1 << DOR1)) != 0) {		// overrun error - ignore character
//		DEBUG_LO;
		return;
	}

	if ((sRxError & (1 << FE1)) != 0) {			// framing error - ignore character
//		DEBUG_LO;
		return;
	}

	if (sRxBuf.count == BUFFER_SIZE) {			// buffer full - ignore character
//		DEBUG_LO;
		return;
	}

	// add character to buffer
	*sRxBuf.nextIn = sRxC;
	if (++sRxBuf.nextIn == sRxBuf.buffer + BUFFER_SIZE)
		sRxBuf.nextIn = sRxBuf.buffer;	// wraparound
	sRxBuf.count++;

#ifdef DEBUG
	if (sRxBuf.count > sRxBuf.maxCount)
		sRxBuf.maxCount = sRxBuf.count;
#endif
//	DEBUG_LO;
}

//-----------------------------------------------------------------------------
/*ISR(SIG_OUTPUT_COMPARE0A) {

	if (sGetchTimeout) 
		sGetchTimeout--;				// used by getch()
}*/

//-----------------------------------------------------------------------------
uint8 UartChars(void) {
	
	return sRxBuf.count;
}

//-----------------------------------------------------------------------------
//	Waits for character to turn up from uart and returns oldest in buffer
//-----------------------------------------------------------------------------
uint8 UartGetch(char *c) {

	// wait for new character to turn up
//	gGetchTimeout = SERIAL_TIMEOUT;
	while (!sRxBuf.count); /* {
		if (!gGetchTimeout) {
			FlushSerialRx();
			return false;				// return false on timeout
		}
	}*/
	
	UCSR1B &= ~(1 << RXCIE1);			// disable the Rx interrupt
	
	// get the oldest character
	*c = *sRxBuf.nextOut;

	// increment nextOut
	if (++sRxBuf.nextOut == sRxBuf.buffer + BUFFER_SIZE)
			sRxBuf.nextOut = sRxBuf.buffer;	// wraparound
	sRxBuf.count--;
	
	UCSR1B |= (1 << RXCIE1);			// enable the Rx interrupt

	return true;
}

//-----------------------------------------------------------------------------
//	Wrapper function to allow use of scanf() etc for host IO
//-----------------------------------------------------------------------------
int UartGet(FILE *stream) {
	
	char c;
	
	if (UartGetch(&c))
		return (int)c;
	else
		return _FDEV_ERR;
}

//-----------------------------------------------------------------------------
void FlushSerial(void) {
	
	FlushSerialRx();
	sTxBuf.nextIn = sTxBuf.nextOut = sTxBuf.buffer;
	sTxBuf.count = 0;
	sTxBuf.busy = false;
}

//-----------------------------------------------------------------------------
void SerialInit(uint8 init) {
	
	sHandleBackspace = false;

	// turn off USART before reconfiguring it
	UCSR1B = 0;
	UCSR1A = 0;
	UCSR1C = 0;
    UART_RELEASE;

    if (!init)
        return;
    
	// Reconfigure the USART in double speed mode for a wider baud rate range at the expense of accuracy
    UART_GRAB;
	UCSR1A = 1 << U2X1;												// double speed mode
	UBRR1 = F_CPU / (8UL * FPGA_BAUD) - 1;							// baud rate - double speed mode	
	UCSR1B = (1 << TXEN1) | (1 << RXCIE1) | (1 << RXEN1); 			// enable tx and rx and rx irq
	UCSR1C = (1 << UCSZ11) | (1 << UCSZ10);							// 8 bits 1 stop

	// init buffers etc
	FlushSerial();
}
