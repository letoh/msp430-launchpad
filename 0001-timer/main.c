#include <msp430g2553.h>
#include <legacymsp430.h>

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;

	CCTL0 = CCIE;
	TACTL = TASSEL_2 + MC_2;  // Set the timer A to SMCLCK, Continuous

	P1DIR |= BIT0 | BIT6;
	P1OUT = 0;

	//__enable_interrupt();
	__bis_SR_register(GIE);

	for(;;)
	{
		__delay_cycles(1000000);
		P1OUT ^= BIT6;
	}
}

interrupt(TIMER0_A0_VECTOR) Timer_A0(void)
{
	P1OUT ^= BIT0;
}

