#ifndef __config_h_
#define __config_h_

#define PROTOCOL_STOPCHAR '\r'

/* ska vara en 2-potens */
#define UART_FIFO_SIZE 32

#define SUART_RX_PORT PIND
#define SUART_RX_DDR  DDRD
#define SUART_RX_PIN  2
//#define SUART_RX_INT  INT0 /* hardcoded for now */
#define SUART_TX_PORT PORTD
#define SUART_TX_DDR  DDRD
#define SUART_TX_PIN  4

#endif
