#include <avr/interrupt.h>

#include "config.h"
#include "system.h"

char real_time[6] = {'0', '0', '0', '0', '0', '0'};

ISR(TIMER1_COMPA_vect) {
  if (real_time[5]++ == '9') {
    real_time[5] = '0';
    if (real_time[4]++ == '5') {
      real_time[4] = '0';
      if (real_time[3]++ == '9') {
        real_time[3] = '0';
        if (real_time[2]++ == '5') {
          real_time[2] = '0';
          if (real_time[1]++ == '9') {
            real_time[1] = '0';
            if (real_time[0]++ == '9') {
              real_time[0] = '0';
            }
          }
        }
      }
    }
  }
  TCNT1H = 0;
  TCNT1L = 0;
}

void set_time(char* buffer) {

}

void init_rtc(void) {
  TCCR1A = 0;
  //System-Clock/1024 => 3600Hz
  TCCR1B = _BV(CS12) | _BV(CS10);
  TCNT1H = 0;
  TCNT1L = 0;
  OCR1AH = 14;
  OCR1AL = 16;
  TIMSK |= _BV(OCIE1A);
}
