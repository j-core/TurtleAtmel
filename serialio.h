//-----------------------------------------------------------------------------
//	Turtle Board Atmel Code
//	Martin Cooper
//	serialio.h
//-----------------------------------------------------------------------------
#ifndef SERIALIO_H_
#define SERIALIO_H_

#include "platform.h"

//#define FPGA_BAUD		38400
#define FPGA_BAUD		250000
#define BUFFER_SIZE		32

#define SERIAL_TIMEOUT	(TICK_FREQ * 30)		// serial timeout

void 		UartPutch(char c);
void 		UartPuts(char *c);
int			UartPut(char c, FILE *stream);
uint8		UartChars(void);
uint8 		UartGetch(char *c);
int			UartGet(FILE *stream);
void 		SerialInit(uint8 init);
void		EmptyTxBuf(void);
uint8 		TxBufFull(void);
void 		FlushSerial(void);
void 		FlushSerialRx(void);


typedef struct {
    volatile char	buffer[BUFFER_SIZE];
    volatile char	*nextIn;
    volatile char	*nextOut;
    volatile uint8	count;
	volatile uint8	busy;
	volatile uint8	maxCount;
} BUFFER;

extern volatile uint16	gGetchTimeout;
extern FILE fuart;


#endif
