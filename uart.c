#include <avr/interrupt.h>

#include "uart.h"
#include "config.h"
#include "system.h"

volatile uart_t uart;

void uart_init()
{
    UCSRC = _BV(URSEL) | _BV(UCSZ1) | _BV(UCSZ0);

    /* baudrate set to 1200 */
    UBRRH = 0x0;
    UBRRL = 0xbf;

    /* enable the transceiver, receiver and Receiver Complete Interrupt */
    UCSRB = _BV(TXEN) | _BV(RXEN) | _BV(RXCIE);
}

void uart_putc(uint8_t byte)
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

uint8_t uart_getc(void)
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
ISR(USART_UDRE_vect)
{
    /* is this the last byte? */
    if((uart.tx.read_pos + 1) % UART_FIFO_SIZE == uart.tx.write_pos)
        UCSRB &= ~_BV(UDRIE);

    /* put the byte into Uart Data Register */
    UDR = uart.tx.data[uart.tx.read_pos];
    uart.tx.read_pos = (uart.tx.read_pos + 1) % UART_FIFO_SIZE;
  
//  PORTC--;
}

ISR(USART_RXC_vect)
{
    if((uart.rx.write_pos + 1) % UART_FIFO_SIZE == uart.rx.read_pos)
    {
        panic("uart: rx full");
    }

    uart.rx.data[uart.rx.write_pos] = UDR;

    if(uart.rx.data[uart.rx.write_pos] == PROTOCOL_STOPCHAR)
        uart.flags.stopchar_received = 1;
    
    uart.rx.write_pos = (uart.rx.write_pos + 1) % UART_FIFO_SIZE;
}

