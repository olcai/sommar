#include <avr/interrupt.h>

#include "suart.h"
#include "config.h"
#include "system.h"

volatile suart_t suart;

void suart_init()
{
}

void suart_putc(uint8_t byte)
{
    /* is there any space in the buffer? */
    if((uart.tx.write_pos + 1) % UART_FIFO_SIZE == uart.tx.read_pos)
    {
        panic("uart: tx full");
    }

    uart.tx.data[uart.tx.write_pos] = byte;
    uart.tx.write_pos = (uart.tx.write_pos + 1) % UART_FIFO_SIZE;

    /* ask the uart to notify us when we can put data into it */
    UCSRB |= _BV(UDRIE);
}

uint8_t suart_getc(void)
{
    uint8_t data;

    /* is the buffer empty */
    if(uart.rx.write_pos == uart.rx.read_pos)
    {
        panic("uart: rx empty");
    }

    data = uart.rx.data[uart.rx.read_pos];
    uart.rx.read_pos = (uart.rx.read_pos + 1) % UART_FIFO_SIZE;
    return data;
}

/* Interrupt Service Routines */

