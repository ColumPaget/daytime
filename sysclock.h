
#ifndef DAYTIME_SYSCLOCK_H
#define DAYTIME_SYSCLOCK_H

#include "common.h"

int UpdateSystemClock(struct timeval *Time);
int UpdateCMOSClock(struct tm *NewTimetm);


#endif
