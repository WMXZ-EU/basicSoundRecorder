#ifndef mTIME_H
#define mTIME_H

#include <inttypes.h>

extern "C" uint32_t rtc_get(void);

typedef struct  { 
  uint8_t Second; 
  uint8_t Minute; 
  uint8_t Hour; 
  uint8_t Wday;   // day of week, sunday is day 1
  uint8_t Day;
  uint8_t Month; 
  uint8_t Year;   // offset from 1970; 
} 	tmElements_t, TimeElements, *tmElementsPtr_t;

//convenience macros to convert to and from tm years 
#define  tmYearToCalendar(Y) ((Y) + 1970)  // full four digit year 
#define  CalendarYrToTm(Y)   ((Y) - 1970)

#define SECS_PER_MIN  ((uint32_t)(60UL))
#define SECS_PER_HOUR ((uint32_t)(3600UL))
#define SECS_PER_DAY  ((uint32_t)(SECS_PER_HOUR * 24UL))


extern int32_t day_ ;
int16_t newDay(void);

extern int32_t hour_ ;
int16_t newHour(void);

/*============================================================================*/	
/* functions to convert to and from system time */
/* These are for interfacing with time services and are not normally needed in a sketch */

// leap year calculator expects year argument as years offset from 1970
#define LEAP_YEAR(Y)   (((1970+(Y))>0) && !((1970+(Y))%4) && ( ((1970+(Y))%100) || !((1970+(Y))%400)))

void breakTime(uint32_t timeInput, tmElements_t &tm);
uint32_t makeTime(const tmElements_t &tm);
void setRTCTime(int hr,int min,int sec,int dy, int mnth, int yr);
#endif
