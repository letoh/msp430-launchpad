#include <msp430.h>

#define CLOCK  1000000L
#define DELAY  (CLOCK / 10)

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
