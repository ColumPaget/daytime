#include "sntp.h"
#include "common.h"
#include <math.h>

#define SNTP_CLIENT 3
#define SNTP_SERVER 4
#define SNTP_BCAST 5
#define PKT_LEN 48
#define TWO_TO_32 4294967296.0

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
uint32_t HashID;
char Hash[64];
char Padding[100];
} SNTPPacket;

double SNTPConvertTime(uint32_t *ptr)
{
double val;

val=(double) ntohl(*ptr);
//this 32 bit value represents seconds since 1900, we need to convert this to
//seconds since 1970
val-=Time_Server_Offset;
ptr++;
val += ((float) ntohl(*ptr)) / TWO_TO_32;

return(val);
}

void SNTPToTimeval(double millisecs, struct timeval *Time)
{
double fractpart, intpart;
int64_t diff;

	fractpart=modf(millisecs, &intpart);
	Time->tv_sec=(time_t) intpart;
	Time->tv_usec=(time_t) (fractpart * 1000000.0);
	diff=diff_millisecs(&TimeNow, Time);
	if (Args->Flags & FLAG_VERBOSE) printf("%ld.%04ld adrift\n", abs((long) diff / 1000), abs((long) diff % 1000));
	if (Args->Flags & FLAG_SYSLOG) syslog(LOG_INFO, "%ld.%04ld adrift", abs((long) diff / 1000), abs((long) diff % 1000));
}


int SNTPHashPacket(SNTPPacket *Packet, int KeyIndex, char **HashStr)
{
char *ptr, *Token=NULL;
THash *Hash;
int len;

ptr=AuthKeyGet(KeyIndex);
if (! StrValid(ptr)) return(0);
ptr=GetToken(ptr,":",&Token,0);
if (! StrValid(ptr)) return(0);
Packet->HashID=1;
Hash=HashInit(Token);
if (! Hash)
{
	DestroyString(Token);
	return(0);
}

Hash->Update(Hash, ptr, StrLen(ptr));
Hash->Update(Hash, (char *) Packet, PKT_LEN);
len=Hash->Finish(Hash, 0, HashStr);
DestroyString(Token);
return(len);
}



int SNTPSignMessage(SNTPPacket *Packet)
{
char *ptr, *Token=NULL;
int len;

len=SNTPHashPacket(Packet, 1, &Token);
memcpy(Packet->Hash, Token, len);
len+=PKT_LEN + sizeof(uint32_t);

DestroyString(Token);
return(len);
}

int SNTPAuthenticateMessage(SNTPPacket *Packet)
{
int len, result=FALSE;
char *Token=NULL;

len=SNTPHashPacket(Packet, 1, &Token);
if (memcmp(Token, Packet->Hash, len)==0) result=TRUE;

DestroyString(Token);
return(result);
}



SNTPPacket *SNTPReadPacket(STREAM *S, char **Addr, int *Port)
{
SNTPPacket *Packet=NULL;
int len;

if ((S->Timeout) && (FDSelectCentisecs(S->in_fd, SELECT_READ, S->Timeout) < 1)) return(NULL);
Packet=(SNTPPacket *) calloc(1,sizeof(SNTPPacket));
len=UDPRecv(S->in_fd,  (char *) Packet, PKT_LEN, Addr, Port);
if (len < PKT_LEN)
{
free(Packet);
Packet=NULL;
}

return(Packet);
}



void SNTPSetupPacket(SNTPPacket *Packet, const char *RefIdent, int Mode, int Poll, int Stratum)
{
uint32_t secs, fract;

Packet->Leap=0;
Packet->Version = 4;
Packet->Mode=Mode;
Packet->Poll=Poll;
Packet->Stratum=Stratum;
memcpy(Packet->ReferenceIdent,RefIdent,4);

secs=TimeNow.tv_sec + Time_Server_Offset;
fract=(uint32_t) (TWO_TO_32 * ((float) TimeNow.tv_usec / 1000000.0));

if (Mode == SNTP_SERVER)
{
//Packet will either be read from client, or freshly created, in which 
//case these values will be zero
Packet->OriginateTimestamp=Packet->TransmitTimestamp;
Packet->OriginateFraction=Packet->TransmitFraction;

Packet->ReceivedTimestamp=htonl(secs);
Packet->ReceivedFraction=htonl(fract);
}

Packet->TransmitTimestamp=htonl(secs);
Packet->TransmitFraction=htonl(fract);
}





void SNTPSetupBcast(TArgs *Args, const char *Data)
{
char *Token=NULL, *ptr;

Args->Flags |= FLAG_BCAST_SEND;

ptr=GetToken(Data,",",&Token,0);
while (ptr)
{
	ListAddItem(Args->BcastNets,CopyStr(NULL,Token));
	ptr=GetToken(ptr,",",&Token,0);
}

DestroyString(Token);
}



// This is the function that connects to a server and requests time
SNTPPacket *SNTPServerTransact(STREAM *S, const char *Host, int Port)
{
SNTPPacket *Packet=NULL;
ListNode *List=NULL, *Curr=NULL;
char *Src=NULL;
int val;

	//if it takes longer than 10 centisecs to get an ntp reply, give up
	STREAMSetTimeout(S,10);

	//pick an ip address at random (addresses like pool.ntp.org can have many ips)
	//and send a request
	List=LookupHostIPList(Host);
	val=ListSize(List);
	if (val > 0)
	{
	Curr=ListGetNth(List, rand() % val);
	if (Curr)
	{
	Packet=(SNTPPacket *) calloc(1,sizeof(SNTPPacket));
	SNTPSetupPacket(Packet, "\0\0\0\0", SNTP_CLIENT, 0, 0);
	STREAMSendDgram(S, (char *) Curr->Item, Port, (char *) Packet, PKT_LEN);
	free(Packet);

	//read a reply. 
	Packet=SNTPReadPacket(S, &Src, &Port);
	if (Packet && StrValid(Src))
	{
 		if (strcmp(Src, (char *) Curr->Item) !=0)
		{
		if (Args->Flags & FLAG_VERBOSE) printf("ERROR: Request sent to %s. Reply came from %s\n",(char *) Curr->Item, Src);
		free(Packet);
		return(NULL);
		}
		if (Args->Flags & FLAG_VERBOSE) printf("SNTP: reply from server: %s\n", Curr->Item);
	}
	}
	}

	ListDestroy(List, DestroyString);
	DestroyString(Src);
	return(Packet);
}


// This is the function that connects to a server and requests time
int SNTPGetTime(const char *Host, int Port, int Flags, struct timeval *Time)
{
static STREAM *S=NULL;
char *Tempstr=NULL;
int result=FALSE, i;
SNTPPacket *Packet;
double timeval, roundtrip;

//host is a const char ptr so we don't use CopyStr
if (StrLen(Host)==0) Host="pool.ntp.org";

//we can send message from a random port (port '0' means 'pick a random highport')
Tempstr=CopyStr(Tempstr, "udp::0");

S=BindPort(Tempstr, 0);

if (! S)
{
 printf("Failed To Bind UDP Port for SNTP communications\n");
 return(FATAL);
}

//if no host is given, then send to 'pool.ntp.org'
//if we are in 'listen for broadcasts' mode (host=='bcast') then don't send a request

Packet=NULL;
for (i=0; (Packet==NULL) && (i < 10); i++)
{
	Packet=SNTPServerTransact(S, Host, Port);
}

if (Packet)
{
	gettimeofday(&TimeNow, NULL);

	//calculate round trip time
	timeval =  (float) TimeNow.tv_sec + ((float) TimeNow.tv_usec / 1000000.0);
	roundtrip = timeval - SNTPConvertTime(&Packet->OriginateTimestamp);
	roundtrip -= (SNTPConvertTime(&Packet->TransmitTimestamp) - SNTPConvertTime(&Packet->ReceivedTimestamp));

	if (Flags & FLAG_VERBOSE)
	{
	printf("ntp roundtrip: %ld ms\n",(long) ConvertFloatTimeToMillisecs(roundtrip));
	printf("ntp client->server: %ld ms\n", ConvertFloatTimeToMillisecs(SNTPConvertTime(&Packet->ReceivedTimestamp) - SNTPConvertTime(&Packet->OriginateTimestamp)));
	printf("ntp server->client: %ld ms\n", ConvertFloatTimeToMillisecs(timeval - SNTPConvertTime(&Packet->TransmitTimestamp)));
	printf("ntp remote processing: %ld ms\n", ConvertFloatTimeToMillisecs(SNTPConvertTime(&Packet->TransmitTimestamp) - SNTPConvertTime(&Packet->ReceivedTimestamp)));
	}

	SNTPToTimeval(SNTPConvertTime(&Packet->TransmitTimestamp) + (roundtrip / 2), Time);
	result=TRUE;
}
else if (Args->Flags & FLAG_VERBOSE) printf("No NTP response from %s\n",Host);

free(Packet);
DestroyString(Tempstr);

return(result);
}



int SNTPBroadcast(const char *BCastAddr, int Port)
{
STREAM *S;
SNTPPacket *Packet;
int val, len=PKT_LEN;

S=STREAMFromFD(UDPOpen(BCastAddr,0,FALSE));
if (! S)
{
	printf("Failed To Bind UDP Port for SNTP communications\n");
	return(FATAL);
}
val=1;
setsockopt(S->out_fd, SOL_SOCKET, SO_BROADCAST,&val,sizeof(int));

Packet=(SNTPPacket *) calloc(1,sizeof(SNTPPacket));
SNTPSetupPacket(Packet, "0000", SNTP_BCAST, 0, 0);
if (Args->Flags & FLAG_AUTH) len=SNTPSignMessage(Packet);
STREAMSendDgram(S, BCastAddr, Port, (char *) Packet, PKT_LEN);
free(Packet);

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

// This is the function that handles an SNTP request. Here we are acting as a server.
void SNTPServerProcess(STREAM *S, SNTPPacket *Packet, const char *Addr, int Port)
{
uint64_t millis;

if (Packet->Mode==SNTP_CLIENT)
{
	SNTPSetupPacket(Packet, "LOCL", SNTP_SERVER, 10, 0);

	millis=ConvertFloatTimeToMillisecs(SNTPConvertTime(&Packet->ReceivedTimestamp) - SNTPConvertTime(&Packet->OriginateTimestamp));
	if (Args->Flags & FLAG_VERBOSE) printf("SNTP Request from %s. ClockDifference: %ld secs %ld ms\n", Addr, (long) millis / 1000,(long) millis % 1000);
	if (Args->Flags & FLAG_SYSLOG) syslog(LOG_INFO, "SNTP Request from %s. ClockDifference: %ld secs %ld ms\n", Addr, (long) millis / 1000, (long) millis % 1000);

	STREAMSendDgram(S, Addr, Port, Packet, PKT_LEN);
}
}


void SNTPReceive(STREAM *S)
{
struct timeval Time;
SNTPPacket *Packet;
char *Addr=NULL;
int Port;

Packet=SNTPReadPacket(S, &Addr, &Port);
if (Packet)
{
if ((Args->Flags & FLAG_SNTPD) && (Packet->Mode==SNTP_CLIENT)) SNTPServerProcess(S, Packet, Addr, Port);
if ((Args->Flags & FLAG_BCAST_RECV) && (Packet->Mode==SNTP_BCAST))
{
	if (Args->Flags & FLAG_VERBOSE) printf("SNTP BROADCAST from %s\n");
	if (Args->Flags & FLAG_SYSLOG) syslog(LOG_INFO, "SNTP BROADCAST from %s", Addr);

	SNTPToTimeval(SNTPConvertTime(&Packet->TransmitTimestamp), &Time);
	HandleReceivedTime(&Time);
}
}

free(Packet);
DestroyString(Addr);
}
