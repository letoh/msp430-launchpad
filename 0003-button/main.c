#include <msp430.h>

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;

	/* setup button */
	P1OUT |= BIT3;
	P1REN |= BIT3;  /* setup pull-up resistor */

	unsigned char state = P1IN & BIT3;

	P1DIR |= BIT0 | BIT6;
	P1OUT &= ~(BIT0 | BIT6);
	P1OUT |= ~((state) >> 3);

	unsigned short cycle = 0;

	for(;;)
	{
		if(state != (P1IN & BIT3))
		{
			state = P1IN & BIT3;
			P1OUT ^= BIT0;
		}
		if(++cycle >= 1000)
		{
			P1OUT ^= BIT6;
			cycle = 0;
		}
		__delay_cycles(1000);
	}
}

