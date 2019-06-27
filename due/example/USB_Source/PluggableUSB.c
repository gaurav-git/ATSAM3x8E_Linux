/*
 * PluggableUSB.c
 *
 *  Created on: 24-Jun-2019
 *      Author: gaurav
 */
#include "USB_API.h"
#include "USBDesc.h"
#include "USBCore.h"
#include "PluggableUSB.h"

extern uint32_t EndPoints[];
struct _PluggableUSB_ PluggableUSB_;

int getInterface(uint8_t *interfaceCount) {

	int sent = 0;
	struct PluggableUSBModule *node;

	for (node = PluggableUSB_.rootNode; node; node = node->next) {
		int res = node->getInterface(interfaceCount);
		if (res < 0) {
			return -1;
		}
		sent += res;
	}
	return sent;
}

int getDescriptor(USBSetup setup)
{
	struct PluggableUSBModule* node;
	for (node = PluggableUSB_.rootNode; node; node = node->next) {
		int ret = node->getDescriptor(setup);
		// ret!=0 -> request has been processed
		if (ret)
			return ret;
	}
	return 0;
}

void getShortName(char *iSerialNum)
{
	iSerialNum[0] = 'A'+PluggableUSB_.rootNode->pluggedInterface;

	struct PluggableUSBModule* node;
	for (node = PluggableUSB_.rootNode; node; node = node->next) {
		iSerialNum += node->getShortName(iSerialNum);
	}
	*iSerialNum = 0;
}

bool setup(USBSetup setup)
{
	struct PluggableUSBModule* node;
	for (node = PluggableUSB_.rootNode; node; node = node->next) {
		if (node->setup(setup)) {
			return true;
		}
	}
	return false;
}

bool plug(struct PluggableUSBModule *node)
{
	if ((PluggableUSB_.lastEp + node->numEndpoints) > USB_ENDPOINTS) {
		return false;
	}

	if (!PluggableUSB_.rootNode) {
		PluggableUSB_.rootNode = node;
	} else {
		struct PluggableUSBModule *current = PluggableUSB_.rootNode;
		while (current->next) {
			current = current->next;
		}
		current->next = node;
	}

	node->pluggedInterface = PluggableUSB_.lastIf;
	node->pluggedEndpoint = PluggableUSB_.lastEp;
	PluggableUSB_.lastIf += node->numInterfaces;
	for (uint8_t i = 0; i < node->numEndpoints; i++) {
		EndPoints[PluggableUSB_.lastEp] = node->endpointType[i];
		PluggableUSB_.lastEp++;
	}
	return true;
	// restart USB layer???
}

struct _PluggableUSB_ *PluggableUSB_Init(void) {

	PluggableUSB_.lastIf = CDC_ACM_INTERFACE + CDC_INTERFACE_COUNT;
	PluggableUSB_.lastEp = CDC_FIRST_ENDPOINT + CDC_ENPOINT_COUNT;
	PluggableUSB_.rootNode = NULL;
	PluggableUSB_.getInterface = getInterface;
	PluggableUSB_.plug = plug;
	PluggableUSB_.setup = setup;
	PluggableUSB_.getShortName = getShortName;
	PluggableUSB_.getDescriptor = getDescriptor;

	return &PluggableUSB_;

}
