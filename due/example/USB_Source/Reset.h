/*
 * Reset.h
 *
 *  Created on: 24-Jun-2019
 *      Author: gaurav
 */

#ifndef EXAMPLE_USB_SOURCE_RESET_H_
#define EXAMPLE_USB_SOURCE_RESET_H_

#ifdef __cplusplus
extern "C" {
#endif

void initiateReset(int ms);
void tickReset();
void cancelReset();

#ifdef __cplusplus
}
#endif




#endif /* EXAMPLE_USB_SOURCE_RESET_H_ */
