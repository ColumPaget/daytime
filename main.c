#include <time.h>

#include <sys/ioctl.h>
#include <fcntl.h>

#include <syslog.h>
#include "libUseful-2.0/libUseful.h"

#define FATAL -1

//this is the differenct in seconds between when time servers base their
//time, (1900) and when linux bases its time (1970)
#define Time_Server_Offset 2208988800u

#define FLAG_DAYTIME 1
#define FLAG_TIME 2
#define FLAG_HTTP 4
#define FLAG_NIST 8
#define FLAG_SNTP 16
#define FLAG_SNTP_BCAST 32
#define FLAG_DEMON 64
#define FLAG_BACKGROUND 128
#define FLAG_SETSYS 4096
#define FLAG_SETRTC 8192

char *Version="0.5";
char *OldTimeZone=NULL, *CurrTimeZone=NULL;

typedef struct
{
int Flags;
char *Host;
int Port;
int SleepTime;
ListNode *BcastNets;
} TArgs; 

void SetTimeZone()
{

if (! StrLen(CurrTimeZone)) CurrTimeZone=CopyStr(CurrTimeZone,"");
StripLeadingWhitespace(CurrTimeZone);
StripTrailingWhitespace(CurrTimeZone);
setenv("TZ",CurrTimeZone,TRUE);
}


void ParseHostAndPort(char *Arg, char **Host, int *Port)
{
char *ptr;

*Port=0;

ptr=strchr(Arg,':');
if (ptr)
{
*ptr='\0';
ptr++;
*Port=atoi(ptr);
}

*Host=CopyStr(*Host,Arg);
}



int GetTimeFromDayTimeHost(char *HostName, int port, struct tm *NewTimetm)
{
char *Tempstr=NULL, *Line=NULL;
STREAM *S;
int count, result;

if (HostName ==NULL) return;
S=STREAMCreate();
if (! STREAMConnectToHost(S,HostName,port,FALSE))
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
if (! STREAMConnectToHost(S,Tempstr,port,FALSE))
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
int sock, result=FALSE;

if (StrLen(HostName) < 1)  return(FALSE);
sock=ConnectToHost(HostName,37,FALSE);
if (sock > -1)
{
Tempstr=FormatStr(Tempstr,"Connected To Time Host %s",HostName);

//we read a 32 bit value from the host
Tempstr=SetStrLen(Tempstr,4);
read(sock,Tempstr,4);

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
if (STREAMConnectToHost(S,Tempstr,80,FALSE))
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


#define VERSION_BIT 8
#define CLIENT_MODE 3
#define BROADCAST_MODE 5
#define PKT_LEN 48


char *BuildSntpPacket(char *Buffer, int Type, time_t LocalTime)
{
char *Data=NULL;
uint32_t *iptr;

Data=SetStrLen(Buffer,PKT_LEN);
memset(Data,0,PKT_LEN);
Data[0] |= VERSION_BIT | Type;

LocalTime+=Time_Server_Offset;

iptr=(uint32_t *) (Data+16);
*iptr=htonl(LocalTime);
iptr=(uint32_t *) (Data+24);
*iptr=htonl(LocalTime);
iptr=(uint32_t *) (Data+40);
*iptr=htonl(LocalTime);


return(Data);
}




int GetTimeFromSNTPHost(char *Host, int Port, int Flags, struct tm *NewTimetm)
{
static STREAM *S=NULL;
char *Data=NULL;
uint32_t *iptr;
int len;
time_t timeval;

time(&timeval);
Data=BuildSntpPacket(Data, CLIENT_MODE, timeval);

if (! S) S=STREAMOpenUDP(Port,FALSE);

//If receiving broadcasts we must bind to the specified port. If not broadcasting
//we can send message from a random port (port '0' means 'pick a random highport')
if (! S && (strcmp(Host,"bcast") !=0)) S=STREAMOpenUDP(0,FALSE);


if (! S)
{
 printf("Failed To Bind UDP Port for SNTP communications\n");
 return(FATAL);
}

//if no host is given, then send to 'pool.ntp.org'
//if we are in 'listen for broadcasts' mode (host=='bcast') then don't send a request

if (StrLen(Host)==0) STREAMSendDgram(S, "pool.ntp.org", Port, Data, PKT_LEN);
else if (strcasecmp(Host,"bcast") !=0) STREAMSendDgram(S, Host, Port, Data, PKT_LEN);

STREAMSetTimeout(S,0);
len=STREAMReadBytes(S, Data, PKT_LEN);
if (len < PKT_LEN)
{
	if (strcmp(Host,"bcast") !=0) printf("No NTP response from %s",Host);
	DestroyString(Data);
 return(FALSE);
}

iptr=(uint32_t *) (Data+40);
timeval=ntohl(*iptr);

//this 32 bit value represents seconds since 1900, we need to convert this to
//seconds since 1970
timeval-=Time_Server_Offset;

memcpy(NewTimetm,gmtime(&timeval),sizeof(struct tm));

DestroyString(Data);

return(TRUE);
}


int SNTPBroadcast(char *BCastAddr, int Port)
{
STREAM *S;
char *Data=NULL, *iptr;
int val;
time_t timeval;

time(&timeval);
Data=BuildSntpPacket(Data, BROADCAST_MODE, timeval);

S=STREAMOpenUDP(BCastAddr,FALSE);
if (! S) S=STREAMOpenUDP(0,FALSE);
if (! S)
{
	printf("Failed To Bind UDP Port for SNTP communications\n");
	return(FATAL);
}
val=1;
setsockopt(S->out_fd, SOL_SOCKET, SO_BROADCAST,&val,sizeof(int));

STREAMSendDgram(S, BCastAddr, Port, Data, PKT_LEN);

DestroyString(Data);
STREAMClose(S);

return(TRUE);
}


int SNTPBroadcastNets(ListNode *Nets)
{
ListNode *Curr;
char *Net=NULL;
int Port=0, result=FALSE;

Curr=ListGetNext(Nets);
while (Curr)
{

ParseHostAndPort((char *) Curr->Item, &Net, &Port);
if (Port==0) Port=123;
result=SNTPBroadcast(Net, Port);

Curr=ListGetNext(Curr);
}

DestroyString(Net);

return(result);
}


int  UpdateSystemClock(struct tm *NewTimetm)
{
time_t NewTime;

NewTime=mktime(NewTimetm);
if (stime(&NewTime)!=0) return(FALSE);

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



//If we are told to broadcast the time using SNTP, do so. This happens here so that we broadcast
//the latest time we got from timeservers above, IF we got a time from a timeserver
//If we didn't get a time, we still run this command to broadcast the system time
if (Args->Flags & FLAG_SNTP_BCAST) result=SNTPBroadcastNets(Args->BcastNets);

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


void PrintServers()
{
printf("daytime,time		time.demon.co.uk\n");
printf("daytime,time		clock.tix.ch\n");
printf("daytime,time		tick.meteonews.ch\n");
printf("nist,time		time.nist.gov\n");
printf("nist,time		time-a.nist.gov\n");
printf("nist,time		time-b.nist.gov\n");
printf("nist,time		time-nw.nist.gov\n");
printf("nist,time		time_a.timefreq.bldrdoc.gov\n");
printf("sntp		pool.ntp.org\n");
printf("time		 clock.psu.edu\n");
printf("time		time.nrc.ca\n");
printf("time		time.chu.nrc.ca\n");
printf("time		time.nrc.ca\n");
printf("time		time.nasa.gov\n");
printf("time		sundial.columbia.edu\n");
printf("http		time.windows.com\n");
printf("http		tycho.usno.navy.mil\n");
printf("http		www.google.com\n");
printf("http		Any well configured webserver!\n");


exit(0);
}


void PrintUsage()
{
fprintf(stdout,"\nDaytime: version %s\n",Version);
fprintf(stdout,"Author: Colum Paget\n");
fprintf(stdout,"Email: colums.projects@gmail.com\n");
fprintf(stdout,"Blogs: \n");
fprintf(stdout,"  tech: http://idratherhack.blogspot.com \n");
fprintf(stdout,"  rants: http://thesingularitysucks.blogspot.com \n");
fprintf(stdout,"\n");
printf("Usage:	daytime -daytime|-time|-nist|-http|-sntp <server> [-s] [-r] [-tz <timezone>] [-p <port>]\n");
printf("	daytime -sntp-bcast <broadcast net>\n");
printf("	-daytime	Get time from daytime (RFC-867, port 13) server\n");
printf("	-time		Get time from time (RFC-868, port 123) server\n");
printf("	-nist		Get time from NIST daytime server\n");
printf("	-http		Get time from a web server\n");
printf("	-sntp		Get time from a SNTP server. (Not full NTP, only accurate to seconds). If server arg is 'bcast' then wait to recieve sntp broadcast.\n");
printf("	-sntp-bcast	Broadcast SNTP Packets to supplied address. This arg can be used multiple times to bcast to multiple nets on a multihomed host\n");
printf("	-d		Daemon mode. Background and stay running. Needed to recieve broadcast times\n");
printf("	-D		Daemon mode WITHOUT BACKGROUNING.\n");
printf("	-t		Sleep time. Time between checks when in daemon mode (default 30 secs).\n");
printf("	-s		Set clock to time we got from server (requires root permissions).\n");
printf("	-r		Set hardware RTC clock\n");
printf("	-p		Port to connect to on server\n");
printf("	-tz		Timezone of server\n");
printf("	-servers	List of some servers to try\n");
printf("	-?		This help\n\n");
printf("If no server specified, http time from www.google.com will be tried first, then nist time time-a.nist.gov then ntp from pool.ntp.org.\n");
printf("\nSNTP Broadcasts and Daemon mode.\n\n");
printf("Receiving the time as an SNTP broadcast requires having daytime stay running and wait for the message. To faciliate this a 'daemon mode' has been added. When -d or -D is used, daytime will stay running and do whatever it was told to do periodically. So:\n\n");
printf("	daytime -t 600 -d -sntp-bcast 192.168.1.255 -sntp-bcast 192.168.2.255\n\n");
printf("Will send sntp broadcasts of the current time to the networks 192.168.1.x and 192.168.2.x. The -t flag can be used to specify a time between broadcasts.\n\n");
printf("	daytime -s -d -sntp bcast\n\n");
printf("Will persist and wait to recieve sntp broadcasts and set the system time from them. NOTE -t cannot be used in sntp broadcast recieve mode\n\n");
printf("	daytime -t 3600 -s -http www.google.com\n\n");
printf("Well check the time with google via http every hour, and set the system time to it\n\n");


printf("\nThanks to Robert Crowley (http://tools.99k.org/) and Andrew Benton for bug reports\n");

exit(0);
}



void SetupSNTPBcast(TArgs *Args, const char *Data)
{
char *Token=NULL, *ptr;

Args->Flags |= FLAG_SNTP_BCAST;

ptr=GetToken(Data,",",&Token,0);
while (ptr)
{
	ListAddItem(Args->BcastNets,CopyStr(NULL,Token));
	ptr=GetToken(ptr,",",&Token,0);
}

DestroyString(Token);
}


void ParseCommandLineArgs(int argc, char *argv[], TArgs *Args)
{
int i;

for (i=1; i < argc; i++)
{
if (strcmp(argv[i],"-s")==0) Args->Flags |= FLAG_SETSYS;
else if (strcmp(argv[i],"-r")==0) Args->Flags |= FLAG_SETRTC;
else if (strcmp(argv[i],"-d")==0) Args->Flags |= FLAG_DEMON | FLAG_BACKGROUND;
else if (strcmp(argv[i],"-D")==0) Args->Flags |= FLAG_DEMON;
else if (strcmp(argv[i],"-daytime")==0) Args->Flags |= FLAG_DAYTIME;
else if (strcmp(argv[i],"-time")==0) Args->Flags |= FLAG_TIME;
else if (strcmp(argv[i],"-nist")==0) Args->Flags |= FLAG_NIST;
else if (strcmp(argv[i],"-http")==0) Args->Flags |= FLAG_HTTP;
else if (strcmp(argv[i],"-sntp")==0) Args->Flags |= FLAG_SNTP;
else if (strcmp(argv[i],"-ntp")==0) Args->Flags |= FLAG_SNTP;
else if (strcmp(argv[i],"-sntp-bcast")==0) SetupSNTPBcast(Args,argv[++i]);
else if (strcmp(argv[i],"-t")==0) Args->SleepTime=atoi(argv[++i]);
else if (strcmp(argv[i],"-tz")==0)
{
	i++;
	CurrTimeZone=CopyStr(CurrTimeZone,argv[i]);
}
else if (strcmp(argv[i],"-?")==0) PrintUsage();
else if (strcmp(argv[i],"-help")==0) PrintUsage();
else if (strcmp(argv[i],"--help")==0) PrintUsage();
else if (strcmp(argv[i],"-h")==0) PrintUsage();
else if (strcmp(argv[i],"-servers")==0) PrintServers();
else 
{
	ParseHostAndPort(argv[i], &Args->Host, &Args->Port);

	//if we are told to listen for broadcasts, then sleep time must be 0
	//otherwise a broadcast will come in while we are sleeping, and when we
	//wake up we will think it's recent when it's not
	if (strcasecmp(Args->Host,"bcast")==0) Args->SleepTime=0;
}
}

}



TArgs *TArgsCreate()
{
TArgs *Args;

Args=(TArgs *) calloc(1,sizeof(TArgs));
Args->BcastNets=ListCreate();
Args->SleepTime=30;

return(Args);
}



main(int argc, char *argv[])
{
char *ptr;
TArgs *Args;
int result;

ptr=getenv("TZ");
if (ptr) ptr=strchr(ptr,'=');
if (ptr) ptr++;
OldTimeZone=CopyStr(OldTimeZone,ptr);
if (StrLen(OldTimeZone)==0) OldTimeZone=CopyStr(OldTimeZone,"");

Args=TArgsCreate();
ParseCommandLineArgs(argc, argv, Args);

if (Args->Flags & FLAG_BACKGROUND) demonize();

result=GoDayTime(Args);
while (Args->Flags & FLAG_DEMON)
{
if (result==FATAL) break;
sleep(Args->SleepTime);
result=GoDayTime(Args);
}

CurrTimeZone=CopyStr(CurrTimeZone,OldTimeZone);
SetTimeZone();
}
