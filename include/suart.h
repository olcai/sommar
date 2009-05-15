#ifndef __suart_h__
#define __suart_h__

#include <stdint.h>
#include "config.h"

/* 
 * write_pos == read_pos => buffer is empty
 * write_pos + 1 == read_pos => buffer is full
 */
   
typedef struct
{
    uint8_t data[UART_FIFO_SIZE];
    uint8_t write_offset; /* offset to byte to write */
    uint8_t write_bit_offset;
    uint8_t read_offset;  /* offset to byte to read  */
    uint8_t read_bit_offset;
} suart_fifo_t;

typedef struct
{
    suart_fifo_t rx;
    suart_fifo_t tx;
    struct
    {
        uint8_t stopchar_received: 1;
    } flags;
} suart_t;

extern volatile suart_t suart;

void suart_init(void);
void suart_putc(uint8_t byte);
uint8_t suart_getc(void);

#endif
