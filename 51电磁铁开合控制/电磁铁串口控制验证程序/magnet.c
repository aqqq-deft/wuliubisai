#include <reg52.h>
#include "magnet.h"

sbit MAGNET_1_PIN = P2^0;
sbit MAGNET_2_PIN = P2^1;
sbit MAGNET_3_PIN = P3^3;

#define MAGNET_1_MASK 0x01u
#define MAGNET_2_MASK 0x02u
#define MAGNET_3_MASK 0x04u

static unsigned char magnet_state_mask = 0u;

void Magnet_Init(void)
{
    /* Write the safe state before any other peripheral is initialized. */
    MAGNET_1_PIN = 1;
    MAGNET_2_PIN = 1;
    MAGNET_3_PIN = 1;
    magnet_state_mask = 0u;
}

unsigned char Magnet_Set(unsigned char channel, unsigned char state)
{
    unsigned char normalized_state;

    normalized_state = (state != MAGNET_OFF) ? MAGNET_ON : MAGNET_OFF;

    switch (channel)
    {
        case 1u:
            MAGNET_1_PIN = (normalized_state == MAGNET_ON) ? 0 : 1;
            if (normalized_state == MAGNET_ON)
            {
                magnet_state_mask |= MAGNET_1_MASK;
            }
            else
            {
                magnet_state_mask &= (unsigned char)(~MAGNET_1_MASK);
            }
            break;

        case 2u:
            MAGNET_2_PIN = (normalized_state == MAGNET_ON) ? 0 : 1;
            if (normalized_state == MAGNET_ON)
            {
                magnet_state_mask |= MAGNET_2_MASK;
            }
            else
            {
                magnet_state_mask &= (unsigned char)(~MAGNET_2_MASK);
            }
            break;

        case 3u:
            MAGNET_3_PIN = (normalized_state == MAGNET_ON) ? 0 : 1;
            if (normalized_state == MAGNET_ON)
            {
                magnet_state_mask |= MAGNET_3_MASK;
            }
            else
            {
                magnet_state_mask &= (unsigned char)(~MAGNET_3_MASK);
            }
            break;

        default:
            return 0u;
    }

    return 1u;
}

void Magnet_All(unsigned char state)
{
    if (state != MAGNET_OFF)
    {
        MAGNET_1_PIN = 0;
        MAGNET_2_PIN = 0;
        MAGNET_3_PIN = 0;
        magnet_state_mask = MAGNET_1_MASK | MAGNET_2_MASK | MAGNET_3_MASK;
    }
    else
    {
        MAGNET_1_PIN = 1;
        MAGNET_2_PIN = 1;
        MAGNET_3_PIN = 1;
        magnet_state_mask = 0u;
    }
}

unsigned char Magnet_Get(unsigned char channel)
{
    unsigned char mask;

    switch (channel)
    {
        case 1u:
            mask = MAGNET_1_MASK;
            break;
        case 2u:
            mask = MAGNET_2_MASK;
            break;
        case 3u:
            mask = MAGNET_3_MASK;
            break;
        default:
            return MAGNET_OFF;
    }

    return ((magnet_state_mask & mask) != 0u) ? MAGNET_ON : MAGNET_OFF;
}
