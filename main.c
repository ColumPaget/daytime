#include "common.h"
#include "sntp.h"
#include "sysclock.h"
#include "command-line-args.h"


void SetTimeZone()
{

if (! StrLen(CurrTimeZone)) CurrTimeZone=CopyStr(CurrTimeZone,"");
StripLeadingWhitespace(CurrTimeZone);
StripTrailingWhitespace(CurrTimeZone);
setenv("TZ",CurrTimeZone,TRUE);
}




int GetTimeFromDayTimeHost(char *HostName, int port, struct timeval *Time)
{
char *Tempstr=NULL, *Line=NULL;
STREAM *S;
int count, result;
struct tm TM;

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
	
	if (strptime(Line,"%a %b %d %H:%M:%S %Y", &TM) !=NULL) 
	{
		Time->tv_sec=mktime(&TM);
		result=TRUE;
	}
	STREAMClose(S);
}
else
{
printf("Failed To Connect to Daytime Host %s\n",HostName);
result=FALSE;
}

Destroy(Tempstr);
Destroy(Line);

return(result);
}


int GetTimeFromNISTHost(char *HostName, int port, struct timeval *Time)
{
char *Tempstr=NULL, *Line=NULL, *ptr;
int count, result;
struct tm TM;
STREAM *S;

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
		if (strptime(ptr,"%y-%m-%d %H:%M:%S",&TM) !=NULL) result=TRUE;
		Time->tv_sec=mktime(&TM);
	}
	STREAMClose(S);
}
else
{
printf("Failed To Connect to NIST Host %s\n",HostName);
result=FALSE;
}

Destroy(Tempstr);
Destroy(Line);

return(result);
}




int GetTimeFromTimeHost(char *HostName, int port, struct timeval *Time)
{
unsigned long timeval;
char *Tempstr=NULL;
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
Time->tv_sec=timeval;
result=TRUE;
}
else
{
	Tempstr=FormatStr(Tempstr,"Failed To Connect To Time Host %s",HostName);
	//Log("TIME: %s",Tempstr);
}
STREAMClose(S);

Destroy(Tempstr);
return(result);
}



int GetTimeFromHTTPHost(char *HostName, int port, struct timeval *Time)
{
char *Tempstr=NULL, *Line=NULL, *Token=NULL, *ptr, *tzptr;
int result=FALSE;
struct tm TM;
STREAM *S;

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

	if (strptime(ptr,"%a, %d %b %Y %H:%M:%S",&TM) !=NULL) 
	{
		Time->tv_sec=mktime(&TM);
		result=TRUE;
	}
  }
  Line=STREAMReadLine(Line,S);
}

}
Destroy(Tempstr);
Destroy(Token);
Destroy(Line);
return(result);
}



int GetTimeFromHost(int Flags, char *Host, int Port, struct timeval *Time)
{
int port=0, result=FALSE;

if (Port > 0) port=Port;

if (Flags & FLAG_TIME)
{
  if (port==0) port=37;
  result=GetTimeFromTimeHost(Host,port,Time);
}
else if (Flags & FLAG_NIST)
{
  if (port==0) port=13;
  result=GetTimeFromNISTHost(Host,port,Time);
}
else if (Flags & FLAG_DAYTIME)
{
  if (port==0) port=13;
	result=GetTimeFromDayTimeHost(Host,port,Time);
}
else if (Flags & FLAG_SNTP) 
{
  if (port==0) port=123;
  result=SNTPGetTime(Host, port, Flags, Time);
}
else if (Flags & FLAG_HTTP)
{
  if (port==0) port=80;
  result=GetTimeFromHTTPHost(Host,port,Time);
}

return(result);
}



int GoDayTime(TArgs *Args)
{
struct timeval Time;
int result=FALSE;

memset(&Time, 0, sizeof(Time));

//If we've not been told to do anything, then try some default
//behaviors
if ((Args->Flags==0) && (StrLen(Args->Host)==0))
{
	result=GetTimeFromHost(FLAG_HTTP, "", 0, &Time);
	if (! result) result=GetTimeFromHost(FLAG_NIST, "", 0, &Time);
	if (! result) result=GetTimeFromHost(FLAG_SNTP, "", 0, &Time);
}
else result=GetTimeFromHost(Args->Flags, Args->Host, Args->Port, &Time);

//if we didn't get a command from any timeserver, then don't print output
if (result == TRUE) HandleReceivedTime(&Time);


return(result);
}



void SetTimeFromCommandLine(TArgs *Args)
{
char *Date=NULL, *Time=NULL, *Tempstr=NULL, *wptr;
const char *DateFormats[]={"%a %b %d %H:%M:%S %Z %Y", "%a %b %d %H:%M:%S %Y", "%b %d %H:%M:%S %Z %Y", "%b %d %H:%M:%S %Y", NULL};
struct tm TM;
struct timeval tv;
const char *ptr;
int i;


if (! isdigit(*Args->SetTime))
{
	tv.tv_usec=0;
	for (i=0; DateFormats[i] != NULL; i++)
	{
		if (strptime(Args->SetTime, DateFormats[i], &TM) != -1) break;
	}
}
else
{
	//if SetTime contains a 'T' followed by a digit then we have
	//the format 1990/10/18T11:10:00
	wptr=strchr(Args->SetTime, 'T');
	if (wptr && isdigit(*(wptr+1))) *wptr=' ';
	
	
	ptr=GetToken(Args->SetTime, " ", &Date, 0);
	ptr=GetToken(ptr, " ", &Time, 0);
	if (strchr(Date, ':')) 
	{
		Tempstr=CopyStr(Tempstr, Time);
		Time=CopyStr(Time, Date);
		Date=CopyStr(Date, Tempstr);
	}
	
	if (StrLen(Time)==0) Time=CopyStr(Time, GetDateStr("%H:%M:%S",NULL));
	else if (StrLen(Time)==5) Time=CatStr(Time, ":00");
	
	if (StrLen(Date)==0) Date=CopyStr(Date, GetDateStr("%Y/%m/%d", NULL));
	else
	{
		ptr=strrchr(Date, '/');
		//we have a 4-digit year at the end of the date
		if (StrLen(ptr) == 5)
		{
			Tempstr=MCopyStr(Tempstr, ptr+1, "/", NULL);
			Tempstr=CatStrLen(Tempstr, Date+3, 2);
			Tempstr=CatStr(Tempstr, "/");
			Tempstr=CatStrLen(Tempstr, Date, 2);
			Date=CopyStr(Date, Tempstr);
		}
		else 
		{
			Tempstr=CopyStrLen(Tempstr, Date, 2);
			Tempstr=CatStr(Tempstr, "/");
			Tempstr=CatStrLen(Tempstr, Date+3, 2);
			Tempstr=CatStr(Tempstr, "/");
			Tempstr=CatStr(Tempstr, Date+6);
			Date=CopyStr(Date, Tempstr);
		}
	
	}

	Tempstr=MCopyStr(Tempstr, Date, " ", Time, NULL);
	printf("Time Parsed As: %s\n",Tempstr);
	strptime(Tempstr, "%Y/%m/%d %H:%M:%S", &TM);
}
	
	
tv.tv_sec=mktime(&TM);
tv.tv_usec=0;

HandleReceivedTime(&tv);


Destroy(Tempstr);
Destroy(Date);
Destroy(Time);
}


int main(int argc, char *argv[])
{
char *ptr;
STREAM *SNTPD=NULL, *S=NULL;
ListNode *Connections;
struct timeval tv;
time_t NextSync=0;
int result, ExitVal=0;

LastError=SetStrLen(LastError, 255);
Connections=ListCreate();
ptr=getenv("TZ");
if (ptr) ptr=strchr(ptr,'=');
if (ptr) ptr++;
OldTimeZone=CopyStr(OldTimeZone,ptr);
if (StrLen(OldTimeZone)==0) OldTimeZone=CopyStr(OldTimeZone,"");

Args=CommandLineParse(argc, argv);

if (Args->Flags & FLAG_CMDLINE_TIME) SetTimeFromCommandLine(Args);
else
{
	if (Args->Flags & FLAG_SYSLOG) openlog("daytime",LOG_PID,0);
	if (Args->Flags & FLAG_BACKGROUND) demonize();

	if (StrValid(Args->PidFilePath)) WritePidFile(Args->PidFilePath);
	if (Args->Flags & (FLAG_SNTPD | FLAG_BCAST_RECV)) 
	{
		SNTPD=BindPort("",123);
		if (SNTPD) ListAddItem(Connections, SNTPD);
	}
	srand(getpid() + time(NULL));

	while (1)
	{
		gettimeofday(&TimeNow, NULL);
		if (TimeNow.tv_sec > NextSync) 
		{
			if (GoDayTime(Args) == FATAL) break;
	
			//If we are told to broadcast the time using SNTP, do so. This happens here so that we broadcast
			//the latest time we got from timeservers above, IF we got a time from a timeserver
			//If we didn't get a time, we still run this command to broadcast the system time
		if (Args->Flags & FLAG_BCAST_SEND) result=SNTPBroadcastNets(Args->BcastNets);
	
		NextSync=TimeNow.tv_sec + Args->SleepTime;
		}
		if (S && (S==SNTPD)) SNTPReceive(S);

		if (! (Args->Flags & FLAG_DEMON)) break;
		tv.tv_sec=Args->SleepTime;
		S=STREAMSelect(Connections, &tv);
	}

	CurrTimeZone=CopyStr(CurrTimeZone,OldTimeZone);
	SetTimeZone();
}

return(ExitVal);
}
