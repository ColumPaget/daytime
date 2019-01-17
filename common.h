#ifndef DAYTIME_COMMON_H
#define DAYTIME_COMMON_H

#include <math.h>

#define _XOPEN_SOURCE
#include <time.h>

#include <sys/ioctl.h>
#include <fcntl.h>

#include <syslog.h>
#include <errno.h>
#include "libUseful-2.5/libUseful.h"


#define VERSION "2.2"

#define FATAL -1

//this is the differenct in seconds between when time servers base their
//time, (1900) and when linux bases its time (1970)
#define Time_Server_Offset 2208988800u

#define FLAG_DAYTIME 1
#define FLAG_TIME 2
#define FLAG_HTTP 4
#define FLAG_NIST 8
#define FLAG_SNTP 16
#define FLAG_SNTPD 32
#define FLAG_BCAST_RECV 64
#define FLAG_BCAST_SEND 128
#define FLAG_BACKGROUND 2048
#define FLAG_SETSYS 4096
#define FLAG_SETRTC 8192
#define FLAG_DEMON 16384
#define FLAG_VERBOSE 32768
#define FLAG_SYSLOG  65536
#define FLAG_AUTH 131072


typedef struct
{
int Flags;
char *Host;
int Port;
int SleepTime;
char *PidFilePath;
ListNode *BcastNets;
} TArgs;

extern char *OldTimeZone, *CurrTimeZone;
extern struct timeval TimeNow;
extern TArgs *Args;
extern char *LastError;
extern int SntpVersion;
extern int SntpStratum;

STREAM *BindPort(const char *URL, int DefaultPort);
long diff_millisecs(struct timeval *t1, struct timeval *t2);
int64_t ConvertFloatTimeToMillisecs(double FloatTime);

void AuthKeySet(const char *KeyInfo);
char *AuthKeyGet(int Index);
void HandleReceivedTime(struct timeval *Time);

#endif
