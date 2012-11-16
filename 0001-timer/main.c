#include <msp430.h>

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

__attribute__((interrupt(TIMER0_A0_VECTOR)))
void timer0_a0_isr(void)
{
	P1OUT ^= BIT0;
}

