/*
 *  USART for AVR ATmega328P
 *  usart.c
 *  by yuhki50 2011/07/26
 */


#include "usart.h"


/* UART‰Šú‰» */
void usart_init(unsigned int baud)
{
    UBRR0 = F_CPU / 16 / baud - 1;
    UCSR0B = _BV(RXEN0) | _BV(TXEN0);

    fdevopen(uart_putchar, NULL);
}


/* UART‚Åˆê•¶š‘—M */
int uart_putchar(char c, FILE* unused)
{
	while(!(UCSR0A & _BV(UDRE0)));
	UDR0 = c;
	return 0;
}
