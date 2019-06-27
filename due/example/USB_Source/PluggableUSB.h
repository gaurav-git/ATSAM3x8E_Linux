/*
 * PluggableUSB.h
 *
 *  Created on: 24-Jun-2019
 *      Author: gaurav
 */

#ifndef EXAMPLE_USB_SOURCE_PLUGGABLEUSB_H_
#define EXAMPLE_USB_SOURCE_PLUGGABLEUSB_H_

#include "USB_API.h"

struct PluggableUSBModule {

	uint8_t pluggedInterface;
	uint8_t pluggedEndpoint;

	const uint8_t numEndpoints;
	const uint8_t numInterfaces;
	const uint32_t *endpointType;

	bool (* setup)(USBSetup setup);

	int (* getInterface)(uint8_t* interfaceCount);

	int (* getDescriptor)(USBSetup setup);

	uint8_t (* getShortName)(char *name);

	struct PluggableUSBModule *next;
};

struct _PluggableUSB_ {

	uint8_t lastIf;
	uint8_t lastEp;
	struct PluggableUSBModule* rootNode;

	bool (* plug)(struct PluggableUSBModule *node);
	int (* getInterface)(uint8_t* interfaceCount);
	int (* getDescriptor)(USBSetup setup);
	bool (* setup)(USBSetup setup);
	void (* getShortName)(char *iSerialNum);
};

struct _PluggableUSB_ *PluggableUSB_Init(void);

#endif /* EXAMPLE_USB_SOURCE_PLUGGABLEUSB_H_ */
