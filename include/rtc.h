#ifndef __RTC_H__
#define __RTC_H__

#include "bool.h"

void rtc_gettime(char* buffer);
bool_t rtc_settime(char* buffer);
void rtc_init(void);

#endif
