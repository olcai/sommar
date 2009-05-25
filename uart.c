#include <avr/interrupt.h>
#include <string.h>

#include "uart.h"
#include "radio.h"
#include "config.h"
#include "system.h"

volatile uart_t uart;

void uart_init()
{
    // TODO: What does this? 
    UCSRC = _BV(URSEL) | _BV(UCSZ1) | _BV(UCSZ0);

    // TODO: Not really needed, we know memory is zero on startup
    //memset(&uart, '\0', sizeof(uart_t));

    /* baudrate set to 1200 */
    UBRRH = 0x0;
    UBRRL = 0xbf;

    /* enable the transceiver and receiver in the uart module */
    /* enable Receive Complete and Transceive Complete Interrupt */
    UCSRB = _BV(TXEN) | _BV(RXEN) | _BV(RXCIE) | _BV(TXCIE);

    /* the radio is connected to us */
    radio_init();
}

bool_t uart_putc(uint8_t byte)
{
    /* is there any space in the buffer? */
    if((uart.tx.write_pos + 1) % UART_FIFO_SIZE == uart.tx.read_pos)
      return FALSE;
    uart.tx.data[uart.tx.write_pos] = byte;
    uart.tx.write_pos = (uart.tx.write_pos + 1) % UART_FIFO_SIZE;

    radio_on();

    /* ask the uart to notify us when we can put data into it */
    UCSRB |= _BV(UDRIE);

    return TRUE;
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
}

ISR(USART_TXC_vect)
{
    /* turn of the radio if the buffer is empty */
    if(uart.tx.read_pos == uart.tx.write_pos)
      radio_off();
}

ISR(USART_RXC_vect)
{
    /* throw away the oldest byte if if the fifo is full */
    if((uart.rx.write_pos + 1) % UART_FIFO_SIZE == uart.rx.read_pos)
      uart.rx.read_pos = (uart.rx.read_pos + 1) % UART_FIFO_SIZE;

    uart.rx.data[uart.rx.write_pos] = UDR;
    if(uart.rx.data[uart.rx.write_pos] == PROTOCOL_STOPCHAR)
        uart.stopchars++;

    uart.rx.write_pos = (uart.rx.write_pos + 1) % UART_FIFO_SIZE;
}

