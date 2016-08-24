#include "common.h"
#include "sntp.h"
#include "command-line-args.h"


void SetTimeZone()
{

if (! StrLen(CurrTimeZone)) CurrTimeZone=CopyStr(CurrTimeZone,"");
StripLeadingWhitespace(CurrTimeZone);
StripTrailingWhitespace(CurrTimeZone);
setenv("TZ",CurrTimeZone,TRUE);
}




int GetTimeFromDayTimeHost(char *HostName, int port, struct tm *NewTimetm)
{
char *Tempstr=NULL, *Line=NULL;
STREAM *S;
int count, result;

if (HostName ==NULL) return(FALSE);
S=STREAMCreate();
if (! STREAMTCPConnect(S,HostName,port,0,0,0))
{
  STREAMClose(S);
  S=NULL;
}


if (S)
{
Line=STREAMReadLine(Line,S);
StripTrailingWhitespace(Line);

SetTimeZone();

if (strptime(Line,"%a %b %d %H:%M:%S %Y",NewTimetm) !=NULL) result=TRUE;
STREAMClose(S);
}
else
{
printf("Failed To Connect to Daytime Host %s\n",HostName);
result=FALSE;
}

DestroyString(Tempstr);
DestroyString(Line);

return(result);
}


int GetTimeFromNISTHost(char *HostName, int port, struct tm *NewTimetm)
{
char *Tempstr=NULL, *Line=NULL, *ptr;
STREAM *S;
int count, result;

if (StrLen(HostName)==0) Tempstr=CopyStr(Tempstr,"time-a.nist.gov");
else Tempstr=CopyStr(Tempstr,HostName);

	
S=STREAMCreate();
if (! STREAMTCPConnect(S,Tempstr,port,0,0,0))
{
  STREAMClose(S);
  S=NULL;
}

if (S)
{
	Line=STREAMReadLine(Line,S);
	while (Line)
	{
	StripTrailingWhitespace(Line);
	if (StrLen(Line) > 0) break;
	Line=STREAMReadLine(Line,S);
	}

	SetTimeZone();

	//Modified Julian Day, but we ignore it
	ptr=GetToken(Line,"\\S",&Tempstr,0);
	if (ptr) 
	{
		if (strptime(ptr,"%y-%m-%d %H:%M:%S",NewTimetm) !=NULL) result=TRUE;
	}
	STREAMClose(S);
}
else
{
printf("Failed To Connect to NIST Host %s\n",HostName);
result=FALSE;
}

DestroyString(Tempstr);
DestroyString(Line);

return(result);
}




int GetTimeFromTimeHost(char *HostName, int port, struct tm *NewTimetm)
{
unsigned long timeval;
char *Tempstr=NULL;
struct tm *TempTimetm=NULL;
int result=FALSE;
STREAM *S;

if (StrLen(HostName) < 1)  return(FALSE);
S=STREAMCreate();
if (STREAMTCPConnect(S,HostName,37,0,0,0))
{
Tempstr=FormatStr(Tempstr,"Connected To Time Host %s",HostName);

//we read a 32 bit value from the host
Tempstr=SetStrLen(Tempstr,4);
STREAMReadBytes(S,Tempstr,4);

memcpy(&timeval,Tempstr,sizeof(long));
timeval=ntohl(timeval);


//this 32 bit value represents seconds since 1900, we need to convert this to
//seconds since 1970
timeval-=Time_Server_Offset;
TempTimetm=gmtime((time_t *)&timeval);

if (TempTimetm !=NULL) *NewTimetm=*TempTimetm;
result=TRUE;
}
else
{
	Tempstr=FormatStr(Tempstr,"Failed To Connect To Time Host %s",HostName);
	//Log("TIME: %s",Tempstr);
}
STREAMClose(S);

DestroyString(Tempstr);
return(result);
}



int GetTimeFromHTTPHost(char *HostName, int port, struct tm *NewTimetm)
{
STREAM *S;
char *Tempstr=NULL, *Line=NULL, *Token=NULL, *ptr, *tzptr;
int result=FALSE;

S=STREAMCreate();

if (StrLen(HostName) ==0) Tempstr=CopyStr(Tempstr,"www.google.com");
else Tempstr=CopyStr(Tempstr,HostName);
if (STREAMTCPConnect(S,Tempstr,80,0,0,0))
{
STREAMWriteLine("HEAD /index.html HTTP/1.0\r\n\r\n",S);
Line=STREAMReadLine(Line,S);

while (Line)
{
  StripTrailingWhitespace(Line);
  ptr=GetToken(Line,":",&Token,0);

  if (strcmp(Token,"Date")==0)
  {
	StripLeadingWhitespace(ptr);

	if (StrLen(CurrTimeZone)==0)
	{
	   tzptr=strrchr(ptr,' ');
	   if (tzptr)
	   {
		tzptr++;
		CurrTimeZone=CopyStr(CurrTimeZone,tzptr);
	   }
	}

	SetTimeZone();

	if (strptime(ptr,"%a, %d %b %Y %H:%M:%S",NewTimetm) !=NULL) result=TRUE;
  }
  Line=STREAMReadLine(Line,S);
}

}
DestroyString(Tempstr);
DestroyString(Token);
DestroyString(Line);
return(result);
}



int UpdateSystemClock(struct tm *NewTimetm)
{
time_t NewTime;
struct timeval tv;

NewTime=mktime(NewTimetm);

#ifdef HAS_STIME
if (stime(&NewTime)!=0) return(FALSE);
#else
tv.tv_sec=NewTime;
tv.tv_usec=0;
settimeofday(&tv, NULL);
#endif

return(TRUE);
}





#ifdef HAVE_RTC_H
#include <linux/rtc.h>
#endif

int  UpdateCMOSClock(struct tm *NewTimetm)
{
int result=TRUE;
#ifdef HAVE_RTC_H
int RTC_DEV_FILE;
struct rtc_time RtcTime;


result=FALSE;
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
	if (ioctl(RTC_DEV_FILE,RTC_SET_TIME,&RtcTime) !=0) perror("Couldn't set Hardware clock: ");
	else result=TRUE;
	close(RTC_DEV_FILE);
}
#endif

return(result);
}


int GetTimeFromHost(int Flags, char *Host, int Port, struct tm *NewTimetm)
{
int port=0, result=FALSE;

if (Port > 0) port=Port;

if (Flags & FLAG_TIME)
{
  if (port==0) port=37;
  result=GetTimeFromTimeHost(Host,port,NewTimetm);
}
else if (Flags & FLAG_NIST)
{
  if (port==0) port=13;
  result=GetTimeFromNISTHost(Host,port,NewTimetm);
}
else if (Flags & FLAG_DAYTIME)
{
  if (port==0) port=13;
	result=GetTimeFromDayTimeHost(Host,port,NewTimetm);
}
else if (Flags & FLAG_SNTP) 
{
  if (port==0) port=123;
  result=GetTimeFromSNTPHost(Host, port, Flags, NewTimetm);
}
else if (Flags & FLAG_HTTP)
{
  if (port==0) port=80;
  result=GetTimeFromHTTPHost(Host,port,NewTimetm);
}

return(result);
}



int GoDayTime(TArgs *Args)
{
int result=FALSE;
struct tm NewTimetm;
char *Tempstr=NULL;


//If we've not been told to do anything, then try some default
//behaviors
if ((Args->Flags==0) && (StrLen(Args->Host)==0))
{
	result=GetTimeFromHost(FLAG_HTTP, "", 0, &NewTimetm);
	if (! result) result=GetTimeFromHost(FLAG_NIST, "", 0, &NewTimetm);
	if (! result) result=GetTimeFromHost(FLAG_SNTP, "", 0, &NewTimetm);
}
else result=GetTimeFromHost(Args->Flags, Args->Host, Args->Port, &NewTimetm);



//if we didn't get a command from any timeserver, then don't print output
if (result !=TRUE) return(result);


printf("\n%s\n",asctime(&NewTimetm));


if (Args->Flags & FLAG_SETSYS)
{
	if (! UpdateSystemClock(&NewTimetm))
	{
		printf("System clock update failed\n");
		syslog(LOG_ERR,"Failed to update system clock");
	}
	else
	{
		printf("System clock set\n");
		syslog(LOG_INFO,"System clock set");
	}
}

if (Args->Flags & FLAG_SETRTC)
{
	if (! UpdateCMOSClock(&NewTimetm))
	{
		printf("Hardware clock update failed\n");
		syslog(LOG_ERR,"Failed to update hardware clock");
	}
	else
	{
		printf("Hardware clock set\n");
		syslog(LOG_INFO,"Hardware clock set");
	}
}

return(result);
}



main(int argc, char *argv[])
{
char *ptr;
TArgs *Args;
STREAM *SNTPD, *S=NULL;
ListNode *Connections;
struct timeval tv;
time_t NextSync=0;
int result;

Connections=ListCreate();
ptr=getenv("TZ");
if (ptr) ptr=strchr(ptr,'=');
if (ptr) ptr++;
OldTimeZone=CopyStr(OldTimeZone,ptr);
if (StrLen(OldTimeZone)==0) OldTimeZone=CopyStr(OldTimeZone,"");

Args=CommandLineParse(argc, argv);

if (Args->Flags & FLAG_BACKGROUND) demonize();

if (StrValid(Args->PidFilePath)) WritePidFile(Args->PidFilePath);
if (Args->Flags & FLAG_SNTPD) 
{
	SNTPD=BindPort("",123);
	ListAddItem(Connections, SNTPD);
}

while (1)
{
	gettimeofday(&TimeNow, NULL);
	if (TimeNow.tv_sec > NextSync) 
	{
		if (GoDayTime(Args) == FATAL) break;

		//If we are told to broadcast the time using SNTP, do so. This happens here so that we broadcast
		//the latest time we got from timeservers above, IF we got a time from a timeserver
		//If we didn't get a time, we still run this command to broadcast the system time
		if (Args->Flags & FLAG_SNTP_BCAST) result=SNTPBroadcastNets(Args->BcastNets);

		NextSync=TimeNow.tv_sec + Args->SleepTime;
	}
	if (S && (S==SNTPD)) SNTPServerProcess(S);

	if (! (Args->Flags & FLAG_DEMON)) break;
	tv.tv_sec=Args->SleepTime;
	S=STREAMSelect(Connections, &tv);
}

CurrTimeZone=CopyStr(CurrTimeZone,OldTimeZone);
SetTimeZone();
}
