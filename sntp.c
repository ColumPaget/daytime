#include "sntp.h"
#include "common.h"


#define VERSION_1 4
#define VERSION_4 16
#define CLIENT_MODE 3
#define SERVER_MODE 4
#define BROADCAST_MODE 5
#define PKT_LEN 48

typedef struct
{
unsigned int Mode:3; 
unsigned int Version:3; 
unsigned int Leap:2; 
uint8_t Stratum; 
uint8_t Poll; 
uint8_t Precision;
uint32_t RootDelay;
uint32_t RootDispersion;
char ReferenceIdent[4];
uint32_t ReferenceTimestamp;
uint32_t ReferenceFraction;
uint32_t OriginateTimestamp;
uint32_t OriginateFraction;
uint32_t ReceivedTimestamp; 
uint32_t ReceivedFraction;
uint32_t TransmitTimestamp; 
uint32_t TransmitFraction;
char Padding[100];
} SNTPPacket;

void SNTPConvertTime(uint32_t *val, time_t *secs, time_t *usecs)
{
*secs=ntohl(*val);
//this 32 bit value represents seconds since 1900, we need to convert this to
//seconds since 1970
(*secs)-=Time_Server_Offset;
val++;
*usecs=ntohl(*val);
}


SNTPPacket *SNTPReadPacket(STREAM *S, char **Addr, int *Port)
{
SNTPPacket *Packet=NULL;
int len;

Packet=(SNTPPacket *) calloc(1,sizeof(SNTPPacket));
len=UDPRecv(S->in_fd,  (char *) Packet, PKT_LEN, Addr, Port);
if (len < PKT_LEN)
{
free(Packet);
Packet=NULL;
}

return(Packet);
}


char *BuildSntpPacket(char *Buffer, int Type, int Version, time_t LocalTime)
{
char *Data=NULL;
uint32_t *iptr;

Data=SetStrLen(Buffer,PKT_LEN);
memset(Data,0,PKT_LEN);
Data[0] |= Version | Type;

LocalTime+=Time_Server_Offset;

iptr=(uint32_t *) (Data+16);
*iptr=htonl(LocalTime);
iptr=(uint32_t *) (Data+24);
*iptr=htonl(LocalTime);
iptr=(uint32_t *) (Data+40);
*iptr=htonl(LocalTime);


return(Data);
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



int GetTimeFromSNTPHost(char *Host, int Port, int Flags, struct tm *NewTimetm)
{
static STREAM *S=NULL;
char *Tempstr=NULL;
uint32_t *iptr;
int len, result=FALSE;
time_t timeval;

if (strcmp(Host,"bcast") ==0) Tempstr=MCopyStr(Tempstr, "udp::" , Port, NULL);
//we can send message from a random port (port '0' means 'pick a random highport')
else Tempstr=CopyStr(Tempstr, "udp::0");

S=BindPort(Tempstr, -1);

if (! S)
{
 printf("Failed To Bind UDP Port for SNTP communications\n");
 return(FATAL);
}

//if no host is given, then send to 'pool.ntp.org'
//if we are in 'listen for broadcasts' mode (host=='bcast') then don't send a request

Tempstr=BuildSntpPacket(Tempstr, CLIENT_MODE, VERSION_1, TimeNow.tv_sec);
if (StrLen(Host)==0) STREAMSendDgram(S, "pool.ntp.org", Port, Tempstr, PKT_LEN);
else if (strcasecmp(Host,"bcast") !=0) STREAMSendDgram(S, Host, Port, Tempstr, PKT_LEN);

STREAMSetTimeout(S,0);
len=STREAMReadBytes(S, Tempstr, PKT_LEN);
if (len >= PKT_LEN)
{
iptr=(uint32_t *) (Tempstr+40);
timeval=ntohl(*iptr);

//this 32 bit value represents seconds since 1900, we need to convert this to
//seconds since 1970
timeval-=Time_Server_Offset;

memcpy(NewTimetm,gmtime(&timeval),sizeof(struct tm));
result=TRUE;
}
else
{
	if (strcmp(Host,"bcast") !=0) printf("No NTP response from %s",Host);
}

DestroyString(Tempstr);

return(result);
}



int SNTPBroadcast(char *BCastAddr, int Port)
{
STREAM *S;
char *Data=NULL, *iptr;
int val;

S=STREAMFromFD(UDPOpen(BCastAddr,0,FALSE));
if (! S)
{
	printf("Failed To Bind UDP Port for SNTP communications\n");
	return(FATAL);
}
val=1;
setsockopt(S->out_fd, SOL_SOCKET, SO_BROADCAST,&val,sizeof(int));

Data=BuildSntpPacket(Data, BROADCAST_MODE, VERSION_1, TimeNow.tv_sec);
STREAMSendDgram(S, BCastAddr, Port, Data, PKT_LEN);

DestroyString(Data);
STREAMClose(S);

return(TRUE);
}


int SNTPBroadcastNets(ListNode *Nets)
{
ListNode *Curr;
char *Net=NULL, *Tempstr=NULL;
int Port=0, result=FALSE;

Curr=ListGetNext(Nets);
while (Curr)
{
ParseURL((char *) Curr->Item, NULL, &Net, &Tempstr, NULL, NULL, NULL, NULL);
if (Tempstr) Port=atoi(Tempstr);
if (Port==0) Port=123;
result=SNTPBroadcast(Net, Port);

Curr=ListGetNext(Curr);
}

DestroyString(Net);
DestroyString(Tempstr);

return(result);
}


void SNTPServerProcess(STREAM *S)
{
char *Addr=NULL;
int Port;
time_t secs, usecs;
SNTPPacket *Packet;

Packet=SNTPReadPacket(S, &Addr, &Port);
if (Packet)
{
SNTPConvertTime(&Packet->TransmitTimestamp, &secs, &usecs);

Packet->Leap=0;
Packet->Version = 4;
Packet->Mode=4;
Packet->Stratum=5;
memcpy(Packet->ReferenceIdent,"LOCL",4);
secs=TimeNow.tv_sec + Time_Server_Offset;
Packet->OriginateTimestamp=Packet->TransmitTimestamp;
Packet->ReceivedTimestamp=htonl(secs);
Packet->TransmitTimestamp=htonl(secs);
STREAMSendDgram(S, Addr, Port, Packet, PKT_LEN);
}

}
