#ifndef __config_h_
#define __config_h_

#include <avr/eeprom.h>
#include <stdint.h>

#define PROTOCOL_STOPCHAR '\n'

/* ska vara en 2-potens */
#define UART_FIFO_SIZE 128

#define SUART_RX_PORT PIND
#define SUART_RX_DDR  DDRD
#define SUART_RX_PIN  2
//#define SUART_RX_INT  INT0 /* hardcoded for now */
#define SUART_TX_PORT PORTD
#define SUART_TX_DDR  DDRD
#define SUART_TX_PIN  4

/* configuration for radio */
#define RADIO_PORT             PORTD
#define RADIO_DDR              DDRD
#define RADIO_PIN              6
#define RADIO_STARTUP_MS_DELAY 6

/*
 * RADIO_POWERUP_COUNTER
 * times of SUART_BIT_TIME_LENGTH to wait for radio to powerup, during this
 * time the data line is high.
 * This is essential numbers of bit lengths to wait before using the
 * radio-tx module.
 *
 * Some empirical values:
 * 10 - Corruption but start to end char is transmitted correctly.
 */
//#define RADIO_POWERUP_COUNTER 10

#define MODE_BUTTON_PORT PIND
#define MODE_BUTTON_DDR  DDRD
#define MODE_BUTTON_PIN  7

//Gruppnummer (1-9)
//#define GROUP_NUMBER 4

#include <stdint.h>

#define CONFIG_MODE_BASE 1
#define CONFIG_MODE_NODE 0
#define CONFIG_GRP_LEN 2

typedef struct
{
  uint16_t interval;
  char group[CONFIG_GRP_LEN+1]; /* + 1 for '\0' */
  struct
  {
    uint8_t mode: 1;
  } flags;
} config_t;

extern volatile config_t config;

void config_load(void);
void config_save(void);

#endif
