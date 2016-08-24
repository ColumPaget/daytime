#ifndef DAYTIME_SNTP
#define DAYTIME_SNTP

#include "common.h"

int GetTimeFromSNTPHost(char *Host, int Port, int Flags, struct tm *NewTimetm);
void SetupSNTPBcast(TArgs *Args, const char *Data);
int SNTPBroadcast(char *BCastAddr, int Port);
int SNTPBroadcastNets(ListNode *Nets);
int SNTPServer(const char *URL);
void SNTPServerProcess(STREAM *S);

#endif
