#include "adc.h"
#include "system.h"
#include <avr/interrupt.h>

static volatile adc_t adc =
{
  0,
  { 0 }
};

void adc_init(void)
{
  ADMUX = _BV(ADLAR);
  ADCSRA = _BV(ADEN) | _BV(ADIE);
}

void adc_dosample(void)
{
  ADCSRA |= _BV(ADSC);
}
/* Interrupt Service Routines */
ISR(ADC_vect)
{
  adc.flags.update_bit = ~adc.flags.update_bit;

 // ADCSRA |= _BV(ADSC);
  PORTC = ADCH;
}
