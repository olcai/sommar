#include <avr/interrupt.h>
#include <string.h>
#include "bool.h"
#include "config.h"
#include "system.h"

typedef struct
{
  char data[6]; /* the time in ASCII */
  struct
  {
    uint8_t update_bit: 1; /* inversed on RTC interrupt */
  } flags;
} rtc_t;

static volatile rtc_t rtc =
{
  { "000000" }, /* start time is 0 */
  { 0 }       /* flags */
};

void rtc_gettime(char* buffer)
{
  uint8_t update_bit;

  do
  {
    update_bit = rtc.flags.update_bit;
    memcpy(buffer, (char*)rtc.data, 6);

    /* if update_bit is not the same then our RTC is now corrupted */
  } while(update_bit != rtc.flags.update_bit);
}

ISR(TIMER1_COMPA_vect)
{
  TCNT1H = 0;
  TCNT1L = 0;
  rtc.flags.update_bit = ~rtc.flags.update_bit;

  /* add the new second and handle the overflow */
  if (rtc.data[5]++ == '9') {
    rtc.data[5] = '0';
    if (rtc.data[4]++ == '5') {
      rtc.data[4] = '0';
      if (rtc.data[3]++ == '9') {
        rtc.data[3] = '0';
        if (rtc.data[2]++ == '5') {
          rtc.data[2] = '0';
          if (rtc.data[1]++ == '9') {
            rtc.data[1] = '0';
            if (rtc.data[0]++ == '9') {
              rtc.data[0] = '0';
            }
          }
        }
      }
    }
  }
}

bool_t rtc_settime(char* buffer) {
  /* ensure that the indata is correct */
  if(buffer[0] >= '0' && buffer[0] <= '9'
     && buffer[1] >= '0' && buffer[1] <= '9'
     && buffer[2] >= '0' && buffer[2] <= '9'
     && buffer[3] >= '0' && buffer[3] <= '9'
     && buffer[4] >= '0' && buffer[4] <= '9'
     && buffer[5] >= '0' && buffer[5] <= '9')
  {
    uint8_t update_bit;
    do
    {
      update_bit = rtc.flags.update_bit;
      memcpy((char*)rtc.data, buffer, 6);

      /* if update_bit is not the same then our RTC is now corrupted */
    } while(update_bit != rtc.flags.update_bit);
    return TRUE;
  }

  return FALSE;
}

void rtc_init(void) {
  TCCR1A = 0;
  //System-Clock/1024 => 3600Hz
  TCCR1B = _BV(CS12) | _BV(CS10);
  TCNT1H = 0;
  TCNT1L = 0;
  OCR1AH = 14;
  OCR1AL = 16;
  TIMSK |= _BV(OCIE1A);
}
