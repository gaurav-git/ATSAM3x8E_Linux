/*!
 * @file:printf_lite.h
 * @description:
 **/

#ifndef PRINTF_LITE_H_
#define PRINTF_LITE_H_

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

/***************************************************************************************************
 Commonly used UART macros/Constants
 ***************************************************************************************************/
#define BAUD_RATE 115200
#define Enable_printf_lite  (0) //Macro to enable printf_lite; make it zero to disable prints
#define Enable_UART_TxHexNumber (1)
#define Enable_UART_TxDecimalNumber (1)
#define Enable_UART_TxBinaryNumber  (1)
#define Enable_UART_TxFloatNumber (1)
#define Enable_UART_TxString  (1)

#define C_DefaultDigitsToTransmit_U8          0xffu    // Will transmit the exact digits in the number
#define C_MaxDigitsToTransmit_U8              10u      // Max decimal/hexadecimal digits to be transmitted
#define C_NumOfBinDigitsToTransmit_U8         16u      // Max bits of a binary number to be transmitted
#define C_MaxDigitsToTransmitUsingPrintf_U8   C_DefaultDigitsToTransmit_U8 /* Max dec/hexadecimal digits to be displayed using printf */

#define PRINTF_BUF_SIZ  128

/***************************************************************************************************
                                Macros for Bit Manipulation
 ****************************************************************************************************/
#define  util_GetBitMask(bit)          (1<<(bit))
#define  util_BitSet(x,bit)            ((x) |=  util_GetBitMask(bit))
#define  util_BitClear(x,bit)          ((x) &= ~util_GetBitMask(bit))
#define  util_BitToggle(x,bit)         ((x) ^=  util_GetBitMask(bit))
#define  util_UpdateBit(x,bit,val)     ((val)? util_BitSet(x,bit): util_BitClear(x,bit))


#define  util_GetBitStatus(x,bit)      (((x)&(util_GetBitMask(bit)))!=0u)
#define  util_IsBitSet(x,bit)          (((x)&(util_GetBitMask(bit)))!=0u)
#define  util_IsBitCleared(x,bit)      (((x)&(util_GetBitMask(bit)))==0u)


#define  util_AreAllBitsSet(x,BitMask) (((x)&(BitMask))==BitMask)
#define  util_AreAnyBitsSet(x,BitMask) (((x)&(BitMask))!=0x00u)
/**************************************************************************************************/

/***************************************************************************************************
 Macros for Dec2Ascii,Hec2Ascii and Acsii2Hex conversion
 ****************************************************************************************************/
#define util_Dec2Ascii(Dec)  ((Dec)+0x30)
#define util_Hex2Ascii(Hex) (((Hex)>0x09) ? ((Hex) + 0x37): ((Hex) + 0x30))
#define util_Ascii2Hex(Asc) (((Asc)>0x39) ? ((Asc) - 0x37): ((Asc) - 0x30))
/***************************************************************************************************/
/***************************************************************************************************
 Macros to find the mod of a number
 ***************************************************************************************************/
#define util_GetMod8(dividend,divisor)  (uint8_t) (dividend - (divisor * (uint8_t) (dividend/divisor)))
#define util_GetMod16(dividend,divisor) (uint16_t)(dividend - (divisor * (uint16_t)(dividend/divisor)))
#define util_GetMod32(dividend,divisor) (uint32_t)(dividend - (divisor * (uint32_t)(dividend/divisor)))
/***************************************************************************************************/

void printf_lite(const char *argList, ...);
void printf_lite_init(void);

#endif /* PRINTF_LITE_H_ */
