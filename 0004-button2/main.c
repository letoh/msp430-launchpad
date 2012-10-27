#include <msp430g2553.h>
#include <legacymsp430.h>

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;

	/* setup button */
	P1OUT |= BIT3;
	P1REN |= BIT3;  /* setup pull-up resistor */
	P1IE  |= BIT3;  /* enable interrupt */

	unsigned char state = P1IN & BIT3;

	P1DIR |= BIT0 | BIT6;
	P1OUT &= ~(BIT0 | BIT6);
	P1OUT |= ~((state) >> 3);

	__bis_SR_register(GIE);

	for(;;)
	{
		P1OUT ^= BIT6;
		__delay_cycles(1000000);
	}
}

interrupt(PORT1_VECTOR) DIO_Port1(void)
{
	if(P1IFG & BIT3)
	{
		P1OUT ^= BIT0;
		P1IFG ^= BIT3;
		P1IES ^= BIT3;
	}
}

