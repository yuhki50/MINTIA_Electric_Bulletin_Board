/*
 *  USART for AVR ATmega328P
 *  usart.h
 *  by yuhki50 2011/06/19
 */


#ifndef _USART_H
#define _USART_H


#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>


// プロトタイプ //
void usart_init();
int uart_putchar(char c, FILE* unused);


#endif
