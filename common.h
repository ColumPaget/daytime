#ifndef DAYTIME_COMMON_H
#define DAYTIME_COMMON_H

#define _XOPEN_SOURCE
#include <time.h>

#include <sys/ioctl.h>
#include <fcntl.h>

#include <syslog.h>
#include "libUseful-2.5/libUseful.h"

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
#define FLAG_SNTP_BCAST 64
#define FLAG_BACKGROUND 128
#define FLAG_SETSYS 4096
#define FLAG_SETRTC 8192
#define FLAG_DEMON 16384


typedef struct
{
int Flags;
char *Host;
int Port;
int SleepTime;
char *PidFilePath;
ListNode *BcastNets;
} TArgs;

extern char *Version;
extern char *OldTimeZone, *CurrTimeZone;
extern struct timeval TimeNow;

STREAM *BindPort(const char *URL, int DefaultPort);

#endif
