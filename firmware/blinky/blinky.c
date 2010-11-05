#include "lpc17.h"
#include <stdint.h>

#define USRLED_ON  (FIO0PIN |=  (1 << 11))
#define USRLED_OFF (FIO0PIN &= ~(1 << 11))

/* delay a number of seconds (roughly) */
inline static void wait(uint8_t seconds)
{
	uint32_t i = 400000 * seconds;
	while (--i);
}

int main()
{
	PINSEL0 &= ~((1 << 23) | (1 << 22)); // set as GPIO
	FIO0DIR |= (1 << 11); // set as output (Output = 1, Input = 0)

	while (1) {
		USRLED_ON;
		wait(1);
		USRLED_OFF;
		wait(1);
	}
}
