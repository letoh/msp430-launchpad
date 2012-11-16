#include <msp430.h>

unsigned int analogRead(unsigned int ch)
{
	ADC10CTL1 = ADC10SSEL_0 + ch;
	ADC10CTL0 |= ENC + ADC10SC;
	while (1)
	{
		if ((ADC10CTL1 ^ ADC10BUSY) && (ADC10CTL0 & ADC10IFG))
			break;
	}
	ADC10CTL0 &= ~(ENC + ADC10IFG);
	return ADC10MEM;
}

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;
	/* setup adc10 */
	ADC10CTL0 = ADC10ON + ADC10SHT_2 + SREF_1 + REFON + REF2_5V;

	P1DIR |= BIT6;

	unsigned int i, adcval;
	for(;;)
	{
		for(adcval = 0, i = 0; i < 8; ++i)
		{
			adcval += analogRead(INCH_0);
			__delay_cycles(150);
		}
		adcval >>= 3;
		if(adcval < 500)
			P1OUT |= BIT6;
		else
			P1OUT &= ~BIT6;
		__delay_cycles(100000);
	}
}

