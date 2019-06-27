/*
 * USB_API.h
 *
 *  Created on: 24-Jun-2019
 *      Author: gaurav
 */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "RingBuffer.h"
#include "USBDesc.h"
#include "USB_device.h"
#include "sam3x8e.h"
#include "interrupt_sam_nvic.h"
#include  "component_uotghs.h"
#include "uotghs_device.h"

#ifndef EXAMPLE_USB_SOURCE_USB_API_H_
#define EXAMPLE_USB_SOURCE_USB_API_H_

enum {
	ONE_STOP_BIT = 0,
	ONE_AND_HALF_STOP_BIT = 1,
	TWO_STOP_BITS = 2,
};
enum {
	NO_PARITY = 0,
	ODD_PARITY = 1,
	EVEN_PARITY = 2,
	MARK_PARITY = 3,
	SPACE_PARITY = 4,
};

typedef struct {
        uint8_t bmRequestType;
        uint8_t bRequest;
        uint8_t wValueL;
        uint8_t wValueH;
        uint16_t wIndex;
        uint16_t wLength;
} USBSetup;

struct USBDevice_ {
	bool (* configured)(void);
	bool (* attach)(void);
	bool (* detach)(void);	// Serial port goes down too...
	void (* poll)(void);
};

struct Serial_ {
	struct RingBuffer *_cdc_rx_buffer;
	void (* begin)(uint32_t baud_count);
	void (* begin_config)(uint32_t baud_count, uint8_t config);
	void (* end)(void);

	int (* available)(void);
	int (* availableForWrite)(void);
	void (* accept)(void);
	int (* peek)(void);
	int (* read)(void);
	int (* readb)(void);
	void (* flush)(void);
	size_t (* write_byte)(uint8_t);
	size_t (* write)(const uint8_t *buffer, size_t size);

	// This method allows processing "SEND_BREAK" requests sent by
	// the USB host. Those requests indicate that the host wants to
	// send a BREAK signal and are accompanied by a single uint16_t
	// value, specifying the duration of the break. The value 0
	// means to end any current break, while the value 0xffff means
	// to start an indefinite break.
	// readBreak() will return the value of the most recent break
	// request, but will return it at most once, returning -1 when
	// readBreak() is called again (until another break request is
	// received, which is again returned once).
	// This also mean that if two break requests are received
	// without readBreak() being called in between, the value of the
	// first request is lost.
	// Note that the value returned is a long, so it can return
	// 0-0xffff as well as -1.
	int32_t (*readBreak)(void);

	// These return the settings specified by the USB host for the
	// serial port. These aren't really used, but are offered here
	// in case a sketch wants to act on these settings.
	uint32_t (* baud)(void);
	uint8_t (* stopbits)(void);
	uint8_t (* paritytype)(void);
	uint8_t (* numbits)(void);
	bool (* dtr)(void);
	bool (* rts)(void);
};
extern struct Serial_ SerialUSB;

//================================================================================
//================================================================================
//      MSC 'Driver'

int	MSC_GetInterface(uint8_t* interfaceNum);
int	MSC_GetDescriptor(int i);
bool	MSC_Setup(USBSetup setup);
bool    MSC_Data(uint8_t rx,uint8_t tx);

//================================================================================
//================================================================================
//      CSC 'Driver'

int             CDC_GetInterface(uint8_t* interfaceNum);
int             CDC_GetOtherInterface(uint8_t* interfaceNum);
int             CDC_GetDescriptor(int i);
bool    CDC_Setup(USBSetup setup);

//================================================================================
//================================================================================

#define TRANSFER_RELEASE        0x40
#define TRANSFER_ZERO           0x20

void USBDevice_Init(void);

void USBD_InitControl(int end);
int USBD_SendControl(uint8_t flags, const void* d, uint32_t len);
int USBD_RecvControl(void* d, uint32_t len);
uint8_t USBD_SendInterfaces(void);
bool USBD_ClassInterfaceRequest(USBSetup setup);

uint32_t USBD_Available(uint32_t ep);
uint32_t USBD_SendSpace(uint32_t ep);
uint32_t USBD_Send(uint32_t ep, const void* d, uint32_t len);
uint32_t USBD_Recv(uint32_t ep, void* data, uint32_t len);              // non-blocking
uint32_t USBD_Recv_byte(uint32_t ep);                                                        // non-blocking
void USBD_Flush(uint32_t ep);
uint32_t USBD_Connected(void);


#endif /* EXAMPLE_USB_SOURCE_USB_API_H_ */
