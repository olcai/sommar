#include "adc.h"
#include "bool.h"
#include "system.h"
#include "config.h"
#include "rtc.h"
#include <avr/interrupt.h>

uint16_t sample_counter = 0;


volatile adc_t adc =
{
  { { '0' } },
  { 0 },
  { 0 }
};

void adc_init(void)
{
  ADMUX = _BV(ADLAR);
  ADCSRA = _BV(ADEN) | _BV(ADIE);
  sample_counter = 0;
}

void adc_dosample(void)
{
  ADCSRA |= _BV(ADSC);
}
/* Interrupt Service Routines */
ISR(ADC_vect)
{
  //8-bitars värde, så behövs inte
  //adc.flags.update_bit = ~adc.flags.update_bit;
  
  //Läs in värdet till datavariabeln
  adc.data[0] = ADCH;

  PORTC = adc.data[0];
}

void adc_rtc_tick(void) {
  /* 
   * rtc körs varje sekund, så det blir direktmappning mellan sample_counter
   * och tidsintervallet (som också är i sekunder)
   */
  if (++sample_counter == config.interval) {
    sample_counter = 0;
   
    // nu gör vi det lätt för oss och sparar tiden vi hämtade värdet här
    rtc_gettime((uint8_t*)adc.data_time[0]);

    adc_dosample();
  }
}

bool_t adc_set_interval(const uint8_t* interval) {
  uint16_t temp = 0;
  if (interval[0] == '0') {
    if (interval[1] == '1' && interval[2] == '0' && interval[3] == '0' && interval[4] == '0' && interval[5] == '0') {
      temp = 3600;
      config.interval = temp;
      sample_counter = 0;
      return TRUE;
    }
    else if (interval[1] == '0') {
      if (interval[2] >= '0' && interval[2] <= '5') {
        if (interval[3] >= '0' && interval[3] <= '9') {
          if (interval[4] >= '0' && interval[4] <= '5') {
            if (interval[5] >= '0' && interval[5] <= '9') {
              temp += (interval[2] - '0') * 60 * 10;
              temp += (interval[3] - '0') * 60;
              temp += (interval[4] - '0') * 10;
              temp += (interval[5] - '0');

              config.interval = temp;
              sample_counter = 0;
              return TRUE;
            }
          }
        }
      }
    }
  }
  return FALSE;
}

