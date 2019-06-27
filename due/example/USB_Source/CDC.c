/*
 * CDC.c
 *
 *  Created on: 24-Jun-2019
 *      Author: gaurav
 */

#include "USB_API.h"
#include "USBCore.h"
#include "USBDesc.h"
#include "RingBuffer.h"
#include "Reset.h"


#define CDC_SERIAL_BUFFER_SIZE	512

/* For information purpose only since RTS is not always handled by the terminal application */
#define CDC_LINESTATE_DTR		0x01 // Data Terminal Ready
#define CDC_LINESTATE_RTS		0x02 // Ready to Send

#define CDC_LINESTATE_READY		(CDC_LINESTATE_RTS | CDC_LINESTATE_DTR)

struct ring_buffer
{
	uint8_t buffer[CDC_SERIAL_BUFFER_SIZE];
	volatile uint32_t head;
	volatile uint32_t tail;
}cdc_rx_buffer = { { 0 }, 0, 0};

typedef struct
{
	uint32_t	dwDTERate;
	uint8_t		bCharFormat;
	uint8_t 	bParityType;
	uint8_t 	bDataBits;
	uint8_t		lineState;
} LineInfo;

static volatile LineInfo _usbLineInfo = {
    256000, // dWDTERate
    0x00,  // bCharFormat
    0x00,  // bParityType
    0x08,  // bDataBits
    0x00   // lineState
};

static volatile int32_t breakValue = -1;

_Pragma("pack(1)")
static const CDCDescriptor _cdcInterface =
{
	D_IAD(0,2,CDC_COMMUNICATION_INTERFACE_CLASS,CDC_ABSTRACT_CONTROL_MODEL,1),

	//	CDC communication interface
	D_INTERFACE(CDC_ACM_INTERFACE,1,CDC_COMMUNICATION_INTERFACE_CLASS,CDC_ABSTRACT_CONTROL_MODEL,0),
	D_CDCCS(CDC_HEADER,0x10,0x01),								// Header (1.10 bcd)
	D_CDCCS(CDC_CALL_MANAGEMENT,1,1),							// Device handles call management (not)
	D_CDCCS4(CDC_ABSTRACT_CONTROL_MANAGEMENT,6),				// SET_LINE_CODING, GET_LINE_CODING, SET_CONTROL_LINE_STATE supported
	D_CDCCS(CDC_UNION,CDC_ACM_INTERFACE,CDC_DATA_INTERFACE),	// Communication interface is master, data interface is slave 0
	D_ENDPOINT(USB_ENDPOINT_IN (CDC_ENDPOINT_ACM),USB_ENDPOINT_TYPE_INTERRUPT,0x10, 0x10),

	//	CDC data interface
	D_INTERFACE(CDC_DATA_INTERFACE,2,CDC_DATA_INTERFACE_CLASS,0,0),
	D_ENDPOINT(USB_ENDPOINT_OUT(CDC_ENDPOINT_OUT),USB_ENDPOINT_TYPE_BULK,512,0),
	D_ENDPOINT(USB_ENDPOINT_IN (CDC_ENDPOINT_IN ),USB_ENDPOINT_TYPE_BULK,512,0)
};
static const CDCDescriptor _cdcOtherInterface =
{
	D_IAD(0,2,CDC_COMMUNICATION_INTERFACE_CLASS,CDC_ABSTRACT_CONTROL_MODEL,1),

	//	CDC communication interface
	D_INTERFACE(CDC_ACM_INTERFACE,1,CDC_COMMUNICATION_INTERFACE_CLASS,CDC_ABSTRACT_CONTROL_MODEL,0),
	D_CDCCS(CDC_HEADER,0x10,0x01),								// Header (1.10 bcd)
	D_CDCCS(CDC_CALL_MANAGEMENT,1,1),							// Device handles call management (not)
	D_CDCCS4(CDC_ABSTRACT_CONTROL_MANAGEMENT,6),				// SET_LINE_CODING, GET_LINE_CODING, SET_CONTROL_LINE_STATE supported
	D_CDCCS(CDC_UNION,CDC_ACM_INTERFACE,CDC_DATA_INTERFACE),	// Communication interface is master, data interface is slave 0
	D_ENDPOINT(USB_ENDPOINT_IN (CDC_ENDPOINT_ACM),USB_ENDPOINT_TYPE_INTERRUPT,0x10, 0x10),

	//	CDC data interface
	D_INTERFACE(CDC_DATA_INTERFACE,2,CDC_DATA_INTERFACE_CLASS,0,0),
	D_ENDPOINT(USB_ENDPOINT_OUT(CDC_ENDPOINT_OUT),USB_ENDPOINT_TYPE_BULK,64,0),
	D_ENDPOINT(USB_ENDPOINT_IN (CDC_ENDPOINT_IN ),USB_ENDPOINT_TYPE_BULK,64,0)
};
_Pragma("pack()")

int WEAK CDC_GetInterface(uint8_t* interfaceNum)
{
	interfaceNum[0] += 2;	// uses 2
	return USBD_SendControl(0,&_cdcInterface,sizeof(_cdcInterface));
}

int WEAK CDC_GetOtherInterface(uint8_t* interfaceNum)
{
	interfaceNum[0] += 2;	// uses 2
	return USBD_SendControl(0,&_cdcOtherInterface,sizeof(_cdcOtherInterface));
}

bool WEAK CDC_Setup(USBSetup setup)
{
	uint8_t r = setup.bRequest;
	uint8_t requestType = setup.bmRequestType;

	if (REQUEST_DEVICETOHOST_CLASS_INTERFACE == requestType)
	{
		if (CDC_GET_LINE_CODING == r)
		{
			USBD_SendControl(0,(void*)&_usbLineInfo,7);
			return true;
		}
	}

	if (REQUEST_HOSTTODEVICE_CLASS_INTERFACE == requestType)
	{
		if (CDC_SET_LINE_CODING == r)
		{
			USBD_RecvControl((void*)&_usbLineInfo,7);
			return true;
		}

		if (CDC_SET_CONTROL_LINE_STATE == r)
		{
			_usbLineInfo.lineState = setup.wValueL;
			// auto-reset into the bootloader is triggered when the port, already
			// open at 1200 bps, is closed.
			if (1200 == _usbLineInfo.dwDTERate)
			{
				// We check DTR state to determine if host port is open (bit 0 of lineState).
				if ((_usbLineInfo.lineState & 0x01) == 0)
					initiateReset(250);
				else
					cancelReset();
			}
			return true;
		}

		if (CDC_SEND_BREAK == r)
		{
			breakValue = ((uint16_t)setup.wValueH << 8) | setup.wValueL;
			return true;
		}
	}
	return false;
}

int _serialPeek = -1;

void begin(uint32_t baud_count)
{
	// suppress "unused parameter" warning
	(void)baud_count;
}

void begin_config(uint32_t baud_count, uint8_t config)
{
	// suppress "unused parameter" warning
	(void)baud_count;
	(void)config;
}

void end(void)
{
}

void accept(void)
{
	static uint32_t guard = 0;

	// synchronized access to guard
	do {
		if (__LDREXW(&guard) != 0) {
			__CLREX();
			return;  // busy
		}
	} while (__STREXW(1, &guard) != 0); // retry until write succeed

	struct ring_buffer *buffer = &cdc_rx_buffer;
	uint32_t i = (uint32_t)(buffer->head+1) % CDC_SERIAL_BUFFER_SIZE;
	uint32_t k = 0;
	// if we should be storing the received character into the location
	// just before the tail (meaning that the head would advance to the
	// current location of the tail), we're about to overflow the buffer
	// and so we don't write the character or advance the head.
	while (k != buffer->tail) {
//		uint32_t c;
		if (!USBD_Available(CDC_RX)) {
			udd_ack_fifocon(CDC_RX);
			break;
		}
//		c = USBD_Recv_byte(CDC_RX);
////		 c = UDD_Recv8(CDC_RX & 0xF);
//		buffer->buffer[buffer->head] = c;
//		buffer->head = i;
//
//		i = (i + 1) % CDC_SERIAL_BUFFER_SIZE;
		uint8_t c[CDC_SERIAL_BUFFER_SIZE]; // create temp array
		k = USBD_Recv(CDC_RX, &c, (buffer->tail - i) % CDC_SERIAL_BUFFER_SIZE); // recieve bytes as many as possible into temp array c. k - count of recorded bytes
		uint32_t j;
		for (j=0;j<k;j++) buffer->buffer[(buffer->head + j) % CDC_SERIAL_BUFFER_SIZE] = c[j]; // write k bytes into CDC buffer

		buffer->head = (buffer->head + k) % CDC_SERIAL_BUFFER_SIZE; // change head as head+k
		k = (k + 1) % CDC_SERIAL_BUFFER_SIZE;
	}

	// release the guard
	guard = 0;
}

int available(void)
{
	struct ring_buffer *buffer = &cdc_rx_buffer;
	return (unsigned int)(CDC_SERIAL_BUFFER_SIZE + buffer->head - buffer->tail) % CDC_SERIAL_BUFFER_SIZE;
}

int availableForWrite(void)
{
	// return the number of bytes left in the current bank,
	// always EP size - 1, because bank is flushed on every write
	return (EPX_SIZE - 1);
}

int peek(void)
{
	struct ring_buffer *buffer = &cdc_rx_buffer;

	if (buffer->head == buffer->tail)
	{
		return -1;
	}
	else
	{
		return buffer->buffer[buffer->tail];
	}
}

int read(void)
{
	struct ring_buffer *buffer = &cdc_rx_buffer;

	// if the head isn't ahead of the tail, we don't have any characters
	if (buffer->head == buffer->tail)
	{
		return -1;
	}
	else
	{
		unsigned char c = buffer->buffer[buffer->tail];
		buffer->tail = (unsigned int)(buffer->tail + 1) % CDC_SERIAL_BUFFER_SIZE;
		if (USBD_Available(CDC_RX)) {
			accept();
		}
		return c;
	}
}

int readb(char *c, size_t length)
{
	struct ring_buffer *buffer = &cdc_rx_buffer;

	// if the head isn't ahead of the tail, we don't have any characters
	if (buffer->head == buffer->tail)
	{
		return -1;
	}
	else
	{
		unsigned int d = min((buffer->head - buffer->tail) % CDC_SERIAL_BUFFER_SIZE, length); // know how many bytes we can get from CDC buffer

		unsigned int i;
		for (i = 0; i < d; i++) *c++ = buffer->buffer[(buffer->tail + i) % CDC_SERIAL_BUFFER_SIZE]; // get bytes
		buffer->tail = (unsigned int)(buffer->tail + d) % CDC_SERIAL_BUFFER_SIZE; // change tail as tail+d

		if (USBD_Available(CDC_RX)) {
			accept();
		}
		return c;
	}
}

void flush(void)
{
	USBD_Flush(CDC_TX);
}

size_t write(const uint8_t *buffer, size_t size)
{
	/* only try to send bytes if the high-level CDC connection itself
	 is open (not just the pipe) - the OS should set lineState when the port
	 is opened and clear lineState when the port is closed.
	 bytes sent before the user opens the connection or after
	 the connection is closed are lost - just like with a UART. */

	// TODO - ZE - check behavior on different OSes and test what happens if an
	// open connection isn't broken cleanly (cable is yanked out, host dies
	// or locks up, or host virtual serial port hangs)
	if (_usbLineInfo.lineState > 0)
	{
		int r = USBD_Send(CDC_TX, buffer, size);

		if (r > 0)
		{
			return r;
		} else
		{
			return 0;
		}
	}
	return 0;
}

size_t write_byte(uint8_t c) {
	return write(&c, 1);
}


int32_t readBreak() {
	uint8_t enableInterrupts = ((__get_PRIMASK() & 0x1) == 0 && (__get_FAULTMASK() & 0x1) == 0);

	// disable interrupts,
	// to avoid clearing a breakValue that might occur
	// while processing the current break value
	__disable_irq();

	int ret = breakValue;

	breakValue = -1;

	if (enableInterrupts)
	{
		// re-enable the interrupts
		__enable_irq();
	}

	return ret;
}

unsigned long baud() {
	return _usbLineInfo.dwDTERate;
}

uint8_t stopbits() {
	return _usbLineInfo.bCharFormat;
}

uint8_t paritytype() {
	return _usbLineInfo.bParityType;
}

uint8_t numbits() {
	return _usbLineInfo.bDataBits;
}

bool dtr() {
	return _usbLineInfo.lineState & 0x1;
}

bool rts() {
	return _usbLineInfo.lineState & 0x2;
}

struct Serial_ SerialUSB = {

	.begin = begin,
	.begin_config = begin_config,
	.end = end,
	.available = available,
	.availableForWrite = availableForWrite,
	.accept = accept,
	.peek = peek,
	.read = read,
	.flush = flush,
	.write_byte = write_byte,
	.write = write,
	.readBreak = readBreak,
	.baud = baud,
	.stopbits = stopbits,
	.paritytype = paritytype,
	.numbits = numbits,
	.dtr = dtr,
	.rts = rts,
};
