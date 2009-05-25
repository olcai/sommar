#ifndef __RTC_H__
#define __RTC_H__

#include "bool.h"

void rtc_gettime(uint8_t* buffer);
bool_t rtc_settime(const uint8_t* buffer);
void rtc_init(void);
void rtc_save(void);

#endif
