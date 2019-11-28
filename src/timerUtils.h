/*
    This Timer Utilities is applied to count the **micro-second** of a process block
*/
#ifndef _TIMER_UTILS_H_
#define _TIMER_UTILS_H_

#include <libio.h>
#include <sys/time.h>

typedef struct timeval timeval;

typedef __useconds_t microsecond_t;

extern void _TimerLap(timeval* tv);

extern microsecond_t    TimerInterval_MICRO(timeval* tv_start, timeval* tv_stop);
extern double    TimerInterval_SECOND(timeval* tv_start, timeval* tv_stop);

extern double Mirco2Sec(microsecond_t msecond);
extern double Mirco2Milli(microsecond_t msecond);
#endif // _TIMER_UTILS_H_
