/**
 * @file serial.h
 * 
 * File from https://github.com/tuupola/avr_demo/tree/master/blog/simple_usart
 */

#ifndef _AVR_SERIAL_H_
#define _AVR_SERIAL_H_

#include <stdio.h>
#include <avr/io.h>
#include <util/setbaud.h>

int uart_putchar(char c, FILE *stream);
int uart_getchar(FILE *stream);
void uart_init();


#endif /* _AVR_SERIAL_H_ */