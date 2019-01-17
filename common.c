#include "common.h"

char *OldTimeZone=NULL, *CurrTimeZone=NULL;
struct timeval TimeNow;
char *LastError=NULL;
TArgs *Args=NULL;
ListNode *AuthKeys=NULL;
int SntpVersion=4;
int SntpStratum=3;

STREAM *BindPort(const char *URL, int DefaultPort)
{
	STREAM *S=NULL;
	char *Proto=NULL, *Host=NULL, *Token=NULL, *ptr;
	ListNode *Node;
	int Port, fd=-1;

	ParseURL(URL, &Proto, &Host, &Token, NULL, NULL, NULL, NULL);

//	if (strcmp(Proto, "udp")==0)
	{
		if (StrLen(Token))
		{	
			Port=atoi(Token);
			fd=UDPOpen(Host, Port,0);
		}
		if (fd==-1) fd=UDPOpen(Host, DefaultPort,0);
		if (fd==-1) perror("Failed to bind port: ");
	}


	if (fd > -1)
	{
		S=STREAMFromFD(fd);
	}

	DestroyString(Proto);
	DestroyString(Host);
	DestroyString(Token);
	return(S);
}


long diff_millisecs(struct timeval *t1, struct timeval *t2)
{
long milli;

milli=t1->tv_sec * 1000 + t1->tv_usec / 1000;
milli-=t2->tv_sec * 1000 + t2->tv_usec / 1000;

return(milli);
}


int64_t ConvertFloatTimeToMillisecs(double FloatTime)
{
int64_t Millisecs;
double intpart, fractpart;

fractpart=modf(FloatTime, &intpart);
Millisecs=(int64_t) intpart * 1000;
Millisecs=(int64_t) (fractpart * 1000);

return(Millisecs);
}

void AuthKeySet(const char *KeyInfo)
{
char *ptr, *Tempstr=NULL;
int val;

val=strtol(KeyInfo, &ptr, 10);
if (*ptr==':') ptr++;
Tempstr=FormatStr(Tempstr, "%d", val);
if (! AuthKeys) AuthKeys=ListCreate();
SetVar(AuthKeys, Tempstr, ptr);
DestroyString(Tempstr);
}

char *AuthKeyGet(int Index)
{
char *Tempstr=NULL;
ListNode *Node;
char *ptr;

if (Index < 1) return(NULL);
if (Index==0)
{
	Node=ListGetNext(AuthKeys);
	if (! Node) return(NULL);
	ptr=(char *) Node->Item;
}
else
{
	Tempstr=FormatStr(Tempstr, "%d", Index);
	ptr=GetVar(AuthKeys, Tempstr);
}

DestroyString(Tempstr);
return(ptr);
}


void HandleReceivedTime(struct timeval *Time)
{
char *Tempstr=NULL;
int64_t milli;
struct tm *TM;

TM=localtime(Time);
Tempstr=CopyStr(Tempstr, asctime(TM));
StripTrailingWhitespace(Tempstr);
printf("%s\n",Tempstr);


if (Args->Flags & FLAG_SETSYS)
{
  UpdateSystemClock(Time);
}

if (Args->Flags & FLAG_SETRTC)
{
  UpdateCMOSClock(TM);
}


DestroyString(Tempstr);
}
