#ifndef __MAGNET_H__
#define __MAGNET_H__

#define MAGNET_OFF 0u
#define MAGNET_ON  1u

/*
 * Channel mapping:
 *   1 -> P2.0 -> MOS GND1
 *   2 -> P2.1 -> MOS GND2
 *   3 -> P3.3 -> MOS GND3
 *
 * The MOS input is active low: 0 = ON, 1 = OFF.
 */
void Magnet_Init(void);
unsigned char Magnet_Set(unsigned char channel, unsigned char state);
void Magnet_All(unsigned char state);
unsigned char Magnet_Get(unsigned char channel);

#endif
