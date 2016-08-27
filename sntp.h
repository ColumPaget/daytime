#ifndef DAYTIME_SNTP
#define DAYTIME_SNTP

#include "common.h"

int SNTPGetTime(const char *Host, int Port, int Flags, struct timeval *Time);
void SNTPSetupBcast(TArgs *Args, const char *Data);
int SNTPBroadcast(const char *BCastAddr, int Port);
int SNTPBroadcastNets(ListNode *Nets);
void SNTPReceive(STREAM *S);

#endif
