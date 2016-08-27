#include "sysclock.h"

int UpdateSystemClock(struct timeval *Time)
{
uint64_t usec_diff=0, sec_diff=0;
struct timeval adjust_tm;
int result=FALSE;

/*
if (stime(&NewTime)!=0) return(FALSE);
*/

sec_diff=TimeNow.tv_sec - Time->tv_sec;
if (Time->tv_usec > 0) usec_diff=abs(TimeNow.tv_usec - Time->tv_usec);
if ((sec_diff==0) && (usec_diff==0)) 
{
	printf("Clock already synchroized\n");
	return(TRUE);
}

if (
		(sec_diff ==0) && 
 		(usec_diff < 40000) 
	)
{
	adjust_tm.tv_sec=0;
	adjust_tm.tv_usec=TimeNow.tv_usec - Time->tv_usec;
	if (adjtime(&adjust_tm, NULL) ==0) result=TRUE;
}
else if (settimeofday(Time, NULL) ==0) result=TRUE;

if (! result) 
{
	LastError=CopyStr(LastError, strerror(errno));
	if (Args->Flags & FLAG_SYSLOG) syslog(LOG_ERR, "Failed to update system clock by %ld millisecs. Error was: %s", (long) diff_millisecs(&TimeNow, Time), LastError);
	printf("Failed to update system clock by %ld millisecs. Error was: %s\n", (long) diff_millisecs(&TimeNow, Time), LastError);
}
else
{
	if (Args->Flags & FLAG_SYSLOG) syslog(LOG_INFO, "updated system clock by %ld millisecs", (long) diff_millisecs(&TimeNow, Time));
	printf("updated system clock by %ld millisecs\n", (long) diff_millisecs(&TimeNow, Time));
}
return(TRUE);
}


#ifdef HAVE_RTC_H
#include <linux/rtc.h>
#endif

int  UpdateCMOSClock(struct tm *NewTimetm)
{
int result=FALSE;
#ifdef HAVE_RTC_H
int RTC_DEV_FILE;
struct rtc_time RtcTime;


RTC_DEV_FILE=open("/dev/rtc0",O_RDONLY);
if (RTC_DEV_FILE == -1) RTC_DEV_FILE=open("/dev/rtc",O_RDONLY);
if (RTC_DEV_FILE > -1) 
{
	RtcTime.tm_sec=NewTimetm->tm_sec;
	RtcTime.tm_min=NewTimetm->tm_min;
	RtcTime.tm_hour=NewTimetm->tm_hour;
	RtcTime.tm_mday=NewTimetm->tm_mday;
	RtcTime.tm_mon=NewTimetm->tm_mon;
	RtcTime.tm_year=NewTimetm->tm_year;
	if (ioctl(RTC_DEV_FILE,RTC_SET_TIME,&RtcTime) !=0)
	{
		LastError=CopyStr(LastError, strerror(errno));
		if (Args->Flags & FLAG_SYSLOG) syslog(LOG_ERR, "Failed to update hardware clock. Error was: %s", LastError);
		printf("Failed to update hardware clock. Error was: %s\n", LastError);
	}
	else 
	{
		result=TRUE;
		if (Args->Flags & FLAG_SYSLOG) syslog(LOG_INFO, "updated hardware clock");
		printf("updated hardware clock\n");
	}
	close(RTC_DEV_FILE);
}
#endif

return(result);
}

