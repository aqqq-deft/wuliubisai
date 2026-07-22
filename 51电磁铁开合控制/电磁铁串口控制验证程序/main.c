#include <reg52.h>
#include <string.h>
#include "magnet.h"
#include "uart.h"

static void SendStatus(void)
{
    UART_SendString("M1=");
    UART_SendString((Magnet_Get(1u) == MAGNET_ON) ? "ON" : "OFF");
    UART_SendString(" M2=");
    UART_SendString((Magnet_Get(2u) == MAGNET_ON) ? "ON" : "OFF");
    UART_SendString(" M3=");
    UART_SendString((Magnet_Get(3u) == MAGNET_ON) ? "ON" : "OFF");
    UART_SendString("\r\n");
}

static void SendHelp(void)
{
    UART_SendString("COMMANDS:\r\n");
    UART_SendString("M1 ON | M1 OFF\r\n");
    UART_SendString("M2 ON | M2 OFF\r\n");
    UART_SendString("M3 ON | M3 OFF\r\n");
    UART_SendString("ALL ON | ALL OFF\r\n");
    UART_SendString("STATUS\r\n");
    UART_SendString("HELP\r\n");
}

static void ProcessCommand(const char *command)
{
    if (strcmp(command, "M1 ON") == 0)
    {
        Magnet_Set(1u, MAGNET_ON);
        UART_SendString("OK\r\n");
    }
    else if (strcmp(command, "M1 OFF") == 0)
    {
        Magnet_Set(1u, MAGNET_OFF);
        UART_SendString("OK\r\n");
    }
    else if (strcmp(command, "M2 ON") == 0)
    {
        Magnet_Set(2u, MAGNET_ON);
        UART_SendString("OK\r\n");
    }
    else if (strcmp(command, "M2 OFF") == 0)
    {
        Magnet_Set(2u, MAGNET_OFF);
        UART_SendString("OK\r\n");
    }
    else if (strcmp(command, "M3 ON") == 0)
    {
        Magnet_Set(3u, MAGNET_ON);
        UART_SendString("OK\r\n");
    }
    else if (strcmp(command, "M3 OFF") == 0)
    {
        Magnet_Set(3u, MAGNET_OFF);
        UART_SendString("OK\r\n");
    }
    else if (strcmp(command, "ALL ON") == 0)
    {
        Magnet_All(MAGNET_ON);
        UART_SendString("OK\r\n");
    }
    else if (strcmp(command, "ALL OFF") == 0)
    {
        Magnet_All(MAGNET_OFF);
        UART_SendString("OK\r\n");
    }
    else if (strcmp(command, "STATUS") == 0)
    {
        SendStatus();
    }
    else if (strcmp(command, "HELP") == 0)
    {
        SendHelp();
    }
    else
    {
        UART_SendString("ERR UNKNOWN\r\n");
    }
}

void main(void)
{
    char command[UART_RX_MAX_LENGTH + 1u];
    unsigned char line_result;

    /* This must remain the first hardware initialization call. */
    Magnet_Init();
    UART_Init();
    UART_SendString("READY\r\n");

    while (1)
    {
        line_result = UART_ReadLine(command, sizeof(command));

        if (line_result == UART_LINE_READY)
        {
            ProcessCommand(command);
        }
        else if (line_result == UART_LINE_OVERFLOW)
        {
            UART_SendString("ERR LINE TOO LONG\r\n");
        }
    }
}
