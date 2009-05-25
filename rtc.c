#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <string.h>
#include "bool.h"
#include "config.h"
#include "system.h"
#include "adc.h"

#define RTC_DATA_SIZE 6

typedef struct
{
  uint8_t data[RTC_DATA_SIZE]; /* the time in ASCII */
  struct
  {
    uint8_t update_bit: 1; /* inversed on RTC interrupt */
  } flags;
} rtc_t;

static volatile rtc_t rtc =
{
  { }, /* start time is 0 */
  { 0 }       /* flags */
};

/* storage in eeprom for RTC */
uint8_t EEMEM rtc_eeprom[RTC_DATA_SIZE] = FLASH_TIME;

void rtc_gettime(uint8_t*  buffer)
{
  uint8_t update_bit;

  do
  {
    update_bit = rtc.flags.update_bit;
    memcpy(buffer, (char*)rtc.data, RTC_DATA_SIZE);

    /* if update_bit is not the same then our RTC is now corrupted */
  } while(update_bit != rtc.flags.update_bit);
}

ISR(TIMER1_COMPA_vect)
{
  TCNT1H = 0;
  TCNT1L = 0;
  rtc.flags.update_bit = ~rtc.flags.update_bit;

  adc_rtc_tick();

  /* add the new second and handle the overflow */
  if (rtc.data[5]++ == '9') {
    rtc.data[5] = '0';
    if (rtc.data[4]++ == '5') {
      rtc.data[4] = '0';
      if (rtc.data[3]++ == '9') {
        rtc.data[3] = '0';
        if (rtc.data[2]++ == '5')
        {
          rtc.data[2] = '0';
          if(rtc.data[0] == '2' && ++rtc.data[1] == '4')
            memset(rtc.data, '0', sizeof(rtc.data));
          else if (rtc.data[1] == '9'+1)
          {
            rtc.data[1] = '0';
            rtc.data[0]++;
          }
        }
      }
    }
  }
}

bool_t rtc_settime(const uint8_t* buffer) {
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
      memcpy((char*)rtc.data, buffer, RTC_DATA_SIZE);

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

  /* restore time form eeprom */
  eeprom_read_block((void*)rtc.data, rtc_eeprom, RTC_DATA_SIZE);
}

void rtc_save(void)
{
  eeprom_write_block((void*)rtc.data, rtc_eeprom, RTC_DATA_SIZE);
}
