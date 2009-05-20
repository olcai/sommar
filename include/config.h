#ifndef __config_h_
#define __config_h_

#include <avr/eeprom.h>
#include <stdint.h>

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
#define SUART_RADIO_PORT PORTD
#define SUART_RADIO_DDR  DDRD
#define SUART_RADIO_PIN  5

#define MODE_BUTTON_PORT PIND
#define MODE_BUTTON_DDR  DDRD
#define MODE_BUTTON_PIN  7

//Gruppnummer (1-9)
#define GROUP_NUMBER 4


#define CONFIG_MODE_BASE 1

extern uint8_t EEMEM config;
#define CONFIG (eeprom_read_byte(&config))
#define config_is_set(flag) (CONFIG & _BV(flag))
#define config_set(flags) (eeprom_write_byte(&config, flags))

#endif
