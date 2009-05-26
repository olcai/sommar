#ifndef __radio_h_
#define __radio_h_

#include <util/delay.h>

#include "config.h"

inline void radio_on(void)
{
  /* if it is not already on, we activate it and wait for it to start */
  if(!bit_is_set(RADIO_PORT, RADIO_PIN))
  {
    /* disable the rx-module */
    UCSRB &= ~_BV(RXEN);
    RADIO_PORT |= _BV(RADIO_PIN);
    _delay_ms(RADIO_STARTUP_MS_DELAY);
  }
}

inline void radio_off(void)
{
  RADIO_PORT &= ~_BV(RADIO_PIN);

  /* enable the rx-module */
  UCSRB |= _BV(RXEN);
}

inline void radio_init(void)
{
  /* configure radio-on-pin as output and set low*/
  RADIO_DDR |= _BV(RADIO_PIN);
  radio_off();
}

#endif
