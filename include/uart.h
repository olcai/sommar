#ifndef __uart_h__
#define __uart_h__

#include <stdint.h>

#include "config.h"
#include "bool.h"

/* 
 * write_pos == read_pos => buffer is empty
 * write_pos + 1 == read_pos => buffer is full
 */
   
typedef struct
{
    uint8_t data[UART_FIFO_SIZE];
    uint8_t write_pos; /* offset to byte to write */
    uint8_t read_pos;  /* offset to byte to read  */
} uart_fifo_t;

typedef struct
{
    uart_fifo_t rx;
    uart_fifo_t tx;
    uint8_t stopchars; /* numbers of stopchars in buffer */
} uart_t;

extern volatile uart_t uart;

void uart_init(void);
bool_t uart_putc(uint8_t byte);
uint8_t uart_getc(void);

#endif
