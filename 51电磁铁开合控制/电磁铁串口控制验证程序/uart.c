#include <reg52.h>
#include "uart.h"

#define UART_RX_BUFFER_SIZE (UART_RX_MAX_LENGTH + 1u)

static volatile unsigned char rx_length = 0u;
static volatile unsigned char rx_line_ready = 0u;
static volatile unsigned char rx_overflow_ready = 0u;
static volatile unsigned char rx_discarding = 0u;
static char rx_buffer[UART_RX_BUFFER_SIZE];

void UART_Init(void)
{
    ES = 0;

    rx_length = 0u;
    rx_line_ready = 0u;
    rx_overflow_ready = 0u;
    rx_discarding = 0u;

    /* 11.0592 MHz, UART mode 1, Timer1 mode 2, 9600 baud, SMOD = 0. */
    PCON &= 0x7Fu;
    SCON = 0x50u;
    TMOD = (TMOD & 0x0Fu) | 0x20u;
    TH1 = 0xFDu;
    TL1 = 0xFDu;
    TF1 = 0;
    TI = 0;
    RI = 0;
    TR1 = 1;

    ES = 1;
    EA = 1;
}

void UART_SendChar(char value)
{
    bit previous_serial_interrupt_state;

    previous_serial_interrupt_state = ES;
    ES = 0;

    SBUF = value;
    while (TI == 0)
    {
        /* Wait for the byte to be transmitted. */
    }
    TI = 0;

    ES = previous_serial_interrupt_state;
}

void UART_SendString(const char *text)
{
    bit previous_serial_interrupt_state;

    previous_serial_interrupt_state = ES;
    ES = 0;

    while (*text != '\0')
    {
        SBUF = *text;
        while (TI == 0)
        {
            /* Wait for the byte to be transmitted. */
        }
        TI = 0;
        text++;
    }

    ES = previous_serial_interrupt_state;
}

unsigned char UART_ReadLine(char *output, unsigned char output_size)
{
    unsigned char index;
    unsigned char result;

    if ((output == 0) || (output_size == 0u))
    {
        return UART_LINE_NONE;
    }

    result = UART_LINE_NONE;
    ES = 0;

    if (rx_overflow_ready != 0u)
    {
        rx_overflow_ready = 0u;
        result = UART_LINE_OVERFLOW;
    }
    else if (rx_line_ready != 0u)
    {
        index = 0u;
        while ((index < rx_length) && (index < (unsigned char)(output_size - 1u)))
        {
            output[index] = rx_buffer[index];
            index++;
        }
        output[index] = '\0';

        rx_length = 0u;
        rx_line_ready = 0u;
        result = UART_LINE_READY;
    }

    ES = 1;
    return result;
}

void UART_Interrupt(void) interrupt 4
{
    unsigned char received;

    if (RI != 0)
    {
        RI = 0;
        received = SBUF;

        if ((rx_line_ready == 0u) && (rx_overflow_ready == 0u))
        {
            if (rx_discarding != 0u)
            {
                if ((received == '\r') || (received == '\n'))
                {
                    rx_discarding = 0u;
                    rx_overflow_ready = 1u;
                }
            }
            else if ((received == '\r') || (received == '\n'))
            {
                if (rx_length != 0u)
                {
                    rx_buffer[rx_length] = '\0';
                    rx_line_ready = 1u;
                }
            }
            else if (rx_length < UART_RX_MAX_LENGTH)
            {
                rx_buffer[rx_length] = (char)received;
                rx_length++;
            }
            else
            {
                rx_length = 0u;
                rx_discarding = 1u;
            }
        }
    }

    /* Transmission is normally polled with ES disabled. */
    if (TI != 0)
    {
        TI = 0;
    }
}
