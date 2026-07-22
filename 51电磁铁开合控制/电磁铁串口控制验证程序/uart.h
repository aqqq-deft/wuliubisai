#ifndef __UART_H__
#define __UART_H__

#define UART_RX_MAX_LENGTH 31u

#define UART_LINE_NONE     0u
#define UART_LINE_READY    1u
#define UART_LINE_OVERFLOW 2u

void UART_Init(void);
void UART_SendChar(char value);
void UART_SendString(const char *text);
unsigned char UART_ReadLine(char *output, unsigned char output_size);

#endif
