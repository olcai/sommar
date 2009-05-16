#ifndef __RTC_H__
#define __RTC_H__

extern char real_time[6];

void set_time(char* buffer);
void init_rtc(void);

#endif
