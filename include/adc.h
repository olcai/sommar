#ifndef __SENSOR_PHOTO_H__
#define __SENSOR_PHOTO_H__

#include <stdint.h>

typedef struct
{
  uint8_t data;
  struct
  {
    uint8_t update_bit: 1; /* inversed on RTC interrupt */
  } flags;
} adc_t;

void    adc_init(void);
void    adc_dosample(void);
uint8_t adc_getvalue(void);

#endif
