#ifndef __SENSOR_PHOTO_H__
#define __SENSOR_PHOTO_H__

#include <stdint.h>
#include "bool.h"
#include "rtc.h"

typedef struct
{
  uint8_t data_time[1][RTC_DATA_SIZE];
  uint8_t data[1];
  struct
  {
    uint8_t update_bit: 1; /* inversed on ADC interrupt */
  } flags;
} adc_t;

extern volatile adc_t adc;

void    adc_init(void);
void    adc_dosample(void);
uint8_t adc_getvalue(void);

void adc_rtc_tick(void);
bool_t adc_set_interval(const uint8_t* interval);

#endif
