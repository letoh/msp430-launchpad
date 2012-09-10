#include <msp430g2553.h>
#include <legacymsp430.h>

void uart_init(void);
void uart_send(unsigned char data);
unsigned char uart_recv(void);
void uart_puts(unsigned char *s);

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;

	P1DIR |= BIT0 | BIT6;
	P1OUT &= BIT0 | BIT6;

	uart_init();

	//__enable_interrupt();
	__bis_SR_register(GIE);

	uart_puts("<< hw uart test >>\r\n");
	for(;;)
	{
		__delay_cycles(1000000);
		P1OUT ^= BIT6;
	}
}

/**
 * slau144 15.3 / Table 15.4 (p.435)
 * P1.1 UCA0RXD
 * P1.2 UCA0TXD
 */
void uart_init(void)
{
	UCA0CTL1 |= UCSWRST;
	/* select pin function */
	P1SEL  |= BIT1 | BIT2;
	P1SEL2 |= BIT1 | BIT2;
	/* clock */
	UCA0CTL1 |= UCSSEL_2;  /* SMCLK */
	UCA0BR0 = 104;         /* 1M / 9600 = 104 */
	UCA0BR1 = 0;
	UCA0MCTL = UCBRS0;     /* UCBRSx = 1 */
	UCA0CTL1 &= ~UCSWRST;
	IE2 |= UCA0RXIE;
}

void uart_send(unsigned char data)
{
	while(!(IFG2 & UCA0TXIFG));
	UCA0TXBUF = data;
}

unsigned char uart_recv(void)
{
	while(!(IFG2 & UCA0RXIFG));
	return UCA0RXBUF;
}

void uart_puts(unsigned char *s)
{
	unsigned char ch;
	while(ch = *s++) uart_send(ch);
}

interrupt(USCIAB0RX_VECTOR) usci0rx_isr(void)
{
	char ch = UCA0RXBUF;
	P1OUT ^= BIT0;
	switch(ch)
	{
	case '\r': uart_puts("\r\n"); break;
	case 0x7f: uart_puts("\b \b"); break;
	default: uart_send(ch);
	}
}

