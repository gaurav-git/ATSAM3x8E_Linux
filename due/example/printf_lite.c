/*!
 * @file:printf_lite.c
 * @description:
 * */
#include "printf_lite.h"
#include <string.h>
#include "../include/due_sam3x.init.h"
/*************************************************************************************/

static void UART_TxHexNumber(uint32_t var_hexNumber_u32,
		uint8_t var_numOfDigitsToTransmit_u8) {
	uint8_t i = 0, a[10];

	if (var_hexNumber_u32 == 0) {
		/* If the number zero then update the array with the same for transmitting */
		for (i = 0;
				((i < var_numOfDigitsToTransmit_u8)
						&& (i < C_MaxDigitsToTransmit_U8)); i++)
			a[i] = 0x00;
	} else {
		for (i = 0; i < var_numOfDigitsToTransmit_u8; i++) {
			/* Continue extracting the digits from right side till the Specified var_numOfDigitsToTransmit_u8 */
			if (var_hexNumber_u32 != 0) {
				/* Extract the digits from the number till it becomes zero.
				 First get the lower nibble and shift the number 4 times.
				 If var_number_u32 = 0xABC then extracted digit will be 0x0C and number will become 0xAB.
				 The process continues till it becomes zero or max digits reached*/
				a[i] = (uint8_t) (var_hexNumber_u32 & 0x0f);
				var_hexNumber_u32 = var_hexNumber_u32 >> 4;
			} else if ((var_numOfDigitsToTransmit_u8
					== C_DefaultDigitsToTransmit_U8)
					|| (var_numOfDigitsToTransmit_u8 > C_MaxDigitsToTransmit_U8)) {
				/* Stop the iteration if the Max number of digits are reached or
				 the user expects exact(Default) digits in the number to be transmitted */
				break;
			} else {
				/* In case user expects more digits to be transmitted than the actual digits in number,
				 then update the remaining digits with zero.
				 Ex: var_number_u32 is 0x123 and user wants five digits then 00123 has to be transmitted */
				a[i] = 0x00;
			}
		}
	}

	while (i != 0) {
		/* Finally get the ascii values of the digits and transmit*/
		USART_PutChar(USART0, util_Hex2Ascii(a[i - 1]));
		i--;
	}
}

/*************************************************************************************/

static void UART_TxDecimalNumber(uint32_t var_decNumber_u32,
		uint8_t var_numOfDigitsToTransmit_u8) {
	uint8_t i = 0, a[10];

	if (var_decNumber_u32 == 0) {
		/* If the number is zero then update the array with the same for transmitting */
		for (i = 0;
				((i < var_numOfDigitsToTransmit_u8)
						&& (i < C_MaxDigitsToTransmit_U8)); i++)
			a[i] = 0x00;
	} else {
		for (i = 0; i < var_numOfDigitsToTransmit_u8; i++) {
			/* Continue extracting the digits from right side
			 till the Specified var_numOfDigitsToTransmit_u8 */
			if (var_decNumber_u32 != 0) {
				/* Extract the digits from the number till it becomes zero.
				 First get the remainder and divide the number by 10 each time.
				 If var_number_u32 = 123 then extracted remainder will be 3 and number will be 12.
				 The process continues till it becomes zero or max digits reached*/
				a[i] = util_GetMod32(var_decNumber_u32, 10);
				var_decNumber_u32 = var_decNumber_u32 / 10;

			} else if ((var_numOfDigitsToTransmit_u8
					== C_DefaultDigitsToTransmit_U8)
					|| (var_numOfDigitsToTransmit_u8 > C_MaxDigitsToTransmit_U8)) {
				/* Stop the iteration if the Max number of digits are reached or
				 the user expects exact(Default) digits in the number to be transmitted */
				break;
			} else {
				/*In case user expects more digits to be transmitted than the actual digits in number,
				 then update the remaining digits with zero.
				 Ex: var_number_u32 is 123 and user wants five digits then 00123 has to be transmitted */
				a[i] = 0;
			}
		}
	}

	while (i) {
		/* Finally get the ascii values of the digits and transmit*/
		USART_Write(USART0, util_Dec2Ascii(a[i - 1]), 100);
		i--;
	}
}

/*************************************************************************************/

static void UART_TxFloatNumber(double var_floatNumber_f32) {
	unsigned long var_tempNumber_u32;

	/* Dirty hack to support the floating point by extracting the integer and fractional part.
	 1.Type cast the number to int to get the integer part.
	 2.transmit the extracted integer part followed by a decimal point(.).
	 3.Later the integer part is made zero by subtracting with the extracted integer value.
	 4.Finally the fractional part is multiplied by 100000 to support 6-digit precision */

	var_tempNumber_u32 = (unsigned long) var_floatNumber_f32;
	UART_TxDecimalNumber((uint32_t) var_tempNumber_u32,
			C_DefaultDigitsToTransmit_U8);

	USART_PutChar(USART0, '.');

	var_floatNumber_f32 = var_floatNumber_f32 - var_tempNumber_u32;
	var_tempNumber_u32 = var_floatNumber_f32 * 1000000;
	UART_TxDecimalNumber(var_tempNumber_u32, C_DefaultDigitsToTransmit_U8);
}

/*************************************************************************************/

/***************************************************************************************************
 void  UART_TxBinaryNumber(uint32_t var_binNumber_u32, uint8_t var_numOfBitsToTransmit_u8)
 ***************************************************************************************************
 * Function name:  UART_TxBinaryNumber()
 * I/P Arguments: uint32_t: Hexadecimal Number to be transmitted on UART.
 uint8_t : Number of bits to be transmitted
 * Return value : none
 * description  :This function is used to transmit the binary equivalent of the given number.
 2nd parameter specifies the number of LSB to be transmitted
 The output for the input combinations is as below
 1.(10,4) then 4-LSB will be transmitted ie. 1010
 2.(10,8) then 8-LSB will be transmitted ie. 00001010
 3.(10,2) then 2-LSB will be transmitted ie. 10
 ***************************************************************************************************/
#if (Enable_UART_TxBinaryNumber==1)
static void UART_TxBinaryNumber(uint32_t var_binNumber_u32,
		uint8_t var_numOfBitsToTransmit_u8) {
	uint8_t ch;

	while (var_numOfBitsToTransmit_u8 != 0) {
		/* Start Extracting the bits from the specified bit positions.
		 Get the Acsii values of the bits and transmit */
		ch = util_GetBitStatus(var_binNumber_u32,
				(var_numOfBitsToTransmit_u8 - 1));
		USART_Write(USART0, util_Dec2Ascii(ch), 500);
		var_numOfBitsToTransmit_u8--;
	}
}
#endif
/***************************************************************************************************/

void printf_lite(const char *argList, ...) {
#if (Enable_printf_lite == 1)
	const char *ptr;
	double var_floatNum_f32;
	va_list argp;
	int16_t var_num_s16;
	int32_t var_num_s32;
	uint16_t var_num_u16;
	uint32_t var_num_u32;
	char *str;
	char ch;
	uint8_t var_numOfDigitsToTransmit_u8;

	va_start(argp, argList);
	/* Loop through the list to extract all the input arguments */
	for (ptr = argList; *ptr != '\0'; ptr++) {

		ch = *ptr;
		if (ch == '%') /*Check for '%' as there will be format specifier after it */
		{
			ptr++;
			ch = *ptr;
			if ((ch >= 0x30) && (ch <= 0x39)) {
				var_numOfDigitsToTransmit_u8 = 0;
				while ((ch >= 0x30) && (ch <= 0x39)) {
					var_numOfDigitsToTransmit_u8 = (var_numOfDigitsToTransmit_u8
							* 10) + (ch - 0x30);
					ptr++;
					ch = *ptr;
				}
			} else {
				var_numOfDigitsToTransmit_u8 =
						C_MaxDigitsToTransmitUsingPrintf_U8;
			}
			switch (ch) /* Decode the type of the argument */
			{
			case 'C':
			case 'c': /* Argument type is of char, hence read char data from the argp */
				ch = va_arg(argp, int);
				USART_PutChar(USART0, ch);
				break;

			case 'd': /* Argument type is of signed integer, hence read 16bit data from the argp */
				var_num_s16 = va_arg(argp, int);
#if (Enable_UART_TxDecimalNumber == 1)

				if (var_num_s16 < 0) { /* If the number is -ve then display the 2's complement along with '-' sign */
					var_num_s16 = -var_num_s16;
					USART_PutChar(USART0, '-');
				}
				UART_TxDecimalNumber(var_num_s16, var_numOfDigitsToTransmit_u8);
#endif
				break;

			case 'D': /* Argument type is of integer, hence read 16bit data from the argp */
				var_num_s32 = va_arg(argp, int32_t);
#if (Enable_UART_TxDecimalNumber == 1)
				if (var_num_s32 < 0) { /* If the number is -ve then display the 2's complement along with '-' sign */
					var_num_s32 = -var_num_s32;
					USART_PutChar(USART0, '-');
				}
				UART_TxDecimalNumber(var_num_s32, var_numOfDigitsToTransmit_u8);
#endif
				break;

			case 'u': /* Argument type is of unsigned integer, hence read 16bit unsigned data */
				var_num_u16 = va_arg(argp, int);
#if (Enable_UART_TxDecimalNumber == 1)
				UART_TxDecimalNumber(var_num_u16, var_numOfDigitsToTransmit_u8);
#endif
				break;

			case 'U': /* Argument type is of integer, hence read 32bit unsigend data */
				var_num_u32 = va_arg(argp, uint32_t);
#if (Enable_UART_TxDecimalNumber == 1)
				UART_TxDecimalNumber(var_num_u32, var_numOfDigitsToTransmit_u8);
#endif
				break;

			case 'x': /* Argument type is of hex, hence hexadecimal data from the argp */
				var_num_u16 = va_arg(argp, int);
#if (Enable_UART_TxHexNumber == 1)
				UART_TxHexNumber(var_num_u16, var_numOfDigitsToTransmit_u8);
#endif
				break;

			case 'X': /* Argument type is of hex, hence hexadecimal data from the argp */
				var_num_u32 = va_arg(argp, uint32_t);
#if (Enable_UART_TxHexNumber == 1)
				UART_TxHexNumber(var_num_u32, var_numOfDigitsToTransmit_u8);
#endif
				break;

			case 'b': /* Argument type is of binary,Read int and convert to binary */
				var_num_u16 = va_arg(argp, int);
#if (Enable_UART_TxBinaryNumber == 1)
				if (var_numOfDigitsToTransmit_u8
						== C_MaxDigitsToTransmitUsingPrintf_U8)
					var_numOfDigitsToTransmit_u8 = 16;
				UART_TxBinaryNumber(var_num_u16, var_numOfDigitsToTransmit_u8);
#endif
				break;

			case 'B': /* Argument type is of binary,Read int and convert to binary */
				var_num_u32 = va_arg(argp, uint32_t);
#if (Enable_UART_TxBinaryNumber == 1)
				if (var_numOfDigitsToTransmit_u8
						== C_MaxDigitsToTransmitUsingPrintf_U8)
					var_numOfDigitsToTransmit_u8 = 16;
				UART_TxBinaryNumber(var_num_u32, var_numOfDigitsToTransmit_u8);
#endif
				break;

			case 'F':
			case 'f': /* Argument type is of float, hence read double data from the argp */
				var_floatNum_f32 = va_arg(argp, double);
#if (Enable_UART_TxFloatNumber == 1)
				UART_TxFloatNumber(var_floatNum_f32);
#endif
				break;

			case 'S':
			case 's': /* Argument type is of string, hence get the pointer to sting passed */
				str = va_arg(argp, char *);
#if (Enable_UART_TxString == 1)
				USART_WriteBuffer(USART0, str, strlen(str));
#endif
				break;

			case '%':
				USART_PutChar(USART0, '%');
				break;
			}
		} else {
			/* As '%' is not detected transmit the char passed */
			USART_PutChar(USART0, ch);
		}
	}

	va_end(argp);
#endif
}

/*************************************************************************************/

void printf_lite_init(void) {

#if (Enable_printf_lite == 1)
	pmc_enable_periph_clk(ID_USART0);
	PIO_Configure(PIOA, PIO_PERIPH_A, PIO_PA11A_TXD0, PIO_DEFAULT);
	USART_Configure(USART0, USART_MODE_ASYNCHRONOUS, BAUD_RATE, SystemCoreClock);
	USART_SetTransmitterEnabled(USART0, 1);
#endif
}
