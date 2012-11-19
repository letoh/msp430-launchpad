#include <msp430.h>

#ifndef F_CPU
#define F_CPU  1000000L
#endif
#define DELAY  (F_CPU / 10)

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;
	P1DIR = BIT0 | BIT6;
	P1OUT = 0x0;

	while(1)
	{
		P1OUT ^= BIT0;
		__delay_cycles(DELAY);
		P1OUT ^= BIT6;
		__delay_cycles(DELAY);
	}
}
