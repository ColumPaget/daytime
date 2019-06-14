#include "command-line-args.h"

TArgs *TArgsCreate()
{
TArgs *Args;

Args=(TArgs *) calloc(1,sizeof(TArgs));
Args->BcastNets=ListCreate();
Args->SleepTime=30;

return(Args);
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


void PrintVersion()
{
fprintf(stdout,"\ndaytime (daytime) %s\n",VERSION);
fprintf(stdout,"Copyright (C) 2010 Colum Paget\n");
fprintf(stdout, "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n");
fprintf(stdout,"This is free software: you are free to change and redistribute it.\n");
fprintf(stdout,"There is NO WARRANTY, to the extent permitted by law.\n");
fprintf(stdout, "Written by Colum Paget\n");
exit(0);
}


void PrintUsage()
{
fprintf(stdout,"\nDaytime: version %s\n",VERSION);
fprintf(stdout,"Author: Colum Paget\n");
fprintf(stdout,"Email: colums.projects@gmail.com\n");
fprintf(stdout,"Website: www.cjpaget.co.uk\n");
fprintf(stdout,"\n");
printf("Usage:	daytime -daytime|-time|-nist|-http|-sntp <server> [-s] [-r] [-tz <timezone>] [-P <path>]\n");
printf("	daytime -sntp-bcast <broadcast net> [-sntp-version <version>]\n");
printf("	daytime -sntpd [-sntp-stratum <stratum>] [-sntp-version <version>]\n");
printf("	daytime -S [date or time specifier]\n");
printf("	-daytime	Get time from daytime (RFC-867, port 13) server\n");
printf("	-time		Get time from time (RFC-868, port 123) server\n");
printf("	-nist		Get time from NIST daytime server\n");
printf("	-http		Get time from a web server\n");
printf("	-sntp		Get time from a SNTP server. (Not full NTP, only accurate to seconds). If server arg is 'bcast' then wait to recieve sntp broadcast.\n");
printf("	-sntp-bcast	Broadcast SNTP Packets to supplied address. This arg can be used multiple times to bcast to multiple nets on a multihomed host\n");
printf("	-sntp-version	Version of SNTP in packet. Some devices only accept '1' as the version in sntp broadcasts, when it should be '4'.\n");
printf("	-sntp-stratum	Stratum of SNTP in packet. This is a measure of how many 'hops' away from an atomic clock an ntp server is. Defaults to '3'.\n");
printf("	-sntpd	in Daemon mode (implies -d) and provide an SNTP service.\n");
printf("	-d		Daemon mode. Background and stay running. Needed to recieve broadcast times\n");
printf("	-D		Daemon mode WITHOUT BACKGROUNDING.\n");
printf("	-P		Pidfile path for daemon mode.\n");
printf("	-t		Sleep time. Time between checks when in daemon mode, or between SNTP broadcasts (default 30 secs).\n");
printf("	-s		Set clock to time we got from server (requires root permissions).\n");
printf("	-S		Set clock from the command-line (requires root permissions).\n");
printf("	-r		Set hardware RTC clock\n");
printf("	-v		verbose output (mainly for SNTP mode)\n");
printf("	-l		syslog significant events\n");
printf("	-tz		Timezone of server\n");
printf("	-servers	List of some servers to try\n");
printf("	-?		This help\n\n");
printf("If no server specified, http time from www.google.com will be tried first, then nist time time-a.nist.gov then ntp from pool.ntp.org.\n");
printf("Servers can be specified as a host/port pair, like 'time.somewhere.com:8080'\n");
printf("\nCommand-line set date/time.\n\n");
printf("The '-S' switch allows setting a date or time from the command-line. This can be expressed in one of the following formats:\n\n");
printf("  HH:MM                -  time expressed in hours and minutes, date will stay as current.\n");
printf("  HH:MM:SS             -  time expressed in hours, minutes and seconds, date will stay as current.\n");
printf("  YYYY/mm/dd           -  date expressed in year, month, day. Time will stay as current. \n");
printf("  dd/mm/YYYY           -  date expressed in year, month, day. Time will stay as current. \n");
printf("  YYYY/mm/dd HH:MM:SS  -  date and time. \n");
printf("  dd/mm/YYYY HH:MM:SS  -  date and time. \n");
printf("  HH:MM:SS YYYY/mm/dd  -  date and time. \n");
printf("  HH:MM:SS dd/mm/YYYY  -  date and time. \n");
printf("  YYYY-mm-ddTHH:MM:SS  -  date and time. \n");
printf("  YYYY/mm/ddTHH:MM:SS  -  date and time. \n");
printf("  Sun Jan 20 15:55:37 GMT 2019   -  standard output of the 'date' command\n");
printf("  Sun Jan 20 15:55:37 2019       -  'date' style without zone\n");
printf("  Jan 20 15:55:37 GMT 2019       -  'date' style without day\n");
printf("  Jan 20 15:55:37 2019           -  'date' style without day and zone\n");
printf("\nany character can be used as a separator in date, but time needs to use ':'\n");

printf("\nSNTP Broadcasts and Daemon mode.\n\n");
printf("Receiving the time as an SNTP broadcast requires having daytime stay running and wait for the message. To faciliate this a 'daemon mode' has been added. When -d or -D is used, daytime will stay running and do whatever it was told to do periodically. So:\n\n");
printf("	daytime -t 600 -d -sntp-bcast 192.168.1.255 -sntp-bcast 192.168.2.255\n\n");
printf("Will send sntp broadcasts of the current time to the networks 192.168.1.x and 192.168.2.x. The -t flag can be used to specify a time between broadcasts.\n\n");
printf("	daytime -s -d -sntp bcast\n\n");
printf("Will persist and wait to recieve sntp broadcasts and set the system time from them. NOTE -t cannot be used in sntp broadcast receive mode\n\n");
printf("	daytime -t 3600 -s -http www.google.com\n\n");
printf("Will check the time with google via http every hour, and set the system time to it\n\n");
printf("\nSNTP Server\n\n");
printf("the -sntpd option will put daytime into SNTP server mode, where it will reply to SNTP requests on port 123. This can be combined with other actions, so for example:\n\n");
printf("	daytime -sntpd -sntp-bcast 192.168.2.255 -daytime time.somewhere.com -t 60\n\n");
printf("Will run as an SNTP server, updating time using daytime protocol to 'time.somewhere.com' every 60 seconds and sending sntp broadcasts every 60 seconds too\n");

printf("\nThanks to Robert Crowley (http://tools.99k.org/) and Andrew Benton for bug reports\n");

exit(0);
}



TArgs *CommandLineParse(int argc, char *argv[])
{
TArgs *Args;
char *Tempstr=NULL, *Token=NULL;
int i;

Args=TArgsCreate();
for (i=1; i < argc; i++)
{
if (strcmp(argv[i],"-s")==0) Args->Flags |= FLAG_SETSYS;
else if (strcmp(argv[i],"-S")==0) 
{
	Args->Flags |= FLAG_CMDLINE_TIME | FLAG_SETSYS;
	i++;
	for (; i < argc; i++) Args->SetTime=MCatStr(Args->SetTime, argv[i], " ", NULL);
}
else if (strcmp(argv[i],"-r")==0) Args->Flags |= FLAG_SETRTC;
else if (strcmp(argv[i],"-d")==0) Args->Flags |= FLAG_DEMON | FLAG_BACKGROUND;
else if (strcmp(argv[i],"-v")==0) Args->Flags |= FLAG_VERBOSE;
else if (strcmp(argv[i],"-D")==0) 
{
	Args->Flags |= FLAG_DEMON;
	Args->Flags &= ~FLAG_BACKGROUND;
}
else if (strcmp(argv[i],"-P")==0) Args->PidFilePath=CopyStr(Args->PidFilePath, argv[++i]);
else if (strcmp(argv[i],"-daytime")==0) Args->Flags |= FLAG_DAYTIME;
else if (strcmp(argv[i],"-time")==0) Args->Flags |= FLAG_TIME;
else if (strcmp(argv[i],"-nist")==0) Args->Flags |= FLAG_NIST;
else if (strcmp(argv[i],"-http")==0) Args->Flags |= FLAG_HTTP;
else if (strcmp(argv[i],"-ntp")==0) Args->Flags |= FLAG_SNTP;
else if (strcmp(argv[i],"-sntp")==0) Args->Flags |= FLAG_SNTP;
else if (strcmp(argv[i],"-sntp-version")==0) SntpVersion=atoi(argv[++i]);
else if (strcmp(argv[i],"-sntp-stratum")==0) SntpStratum=atoi(argv[++i]);
else if (strcmp(argv[i],"-sntpd")==0) Args->Flags |= FLAG_SNTPD | FLAG_DEMON | FLAG_BACKGROUND;
else if (strcmp(argv[i],"-sntp-bcast")==0) SNTPSetupBcast(Args,argv[++i]);
else if (strcmp(argv[i],"-t")==0) Args->SleepTime=atoi(argv[++i]);
else if (strcmp(argv[i],"-tz")==0) CurrTimeZone=CopyStr(CurrTimeZone,argv[++i]);
/*
else if (strcmp(argv[i],"-key")==0) 
{
	AuthKeySet(argv[++i]);
	Args->Flags |= FLAG_AUTH;
}
*/
else if (strcmp(argv[i],"-?")==0) PrintUsage();
else if (strcmp(argv[i],"-help")==0) PrintUsage();
else if (strcmp(argv[i],"--help")==0) PrintUsage();
else if (strcmp(argv[i],"-h")==0) PrintUsage();
else if (strcmp(argv[i],"-version")==0) PrintVersion();
else if (strcmp(argv[i],"--version")==0) PrintVersion();
else if (strcmp(argv[i],"-?")==0) PrintUsage();
else if (strcmp(argv[i],"-servers")==0) PrintServers();
else if (strcasecmp(argv[i],"bcast")==0)
{
  Args->Flags &= ~FLAG_SNTP;
  Args->Flags |= FLAG_BCAST_RECV;
}
else 
{
	Tempstr=MCopyStr(Tempstr, "tcp:",argv[i],NULL);
	ParseURL(Tempstr, NULL, &Args->Host, &Token, NULL, NULL, NULL, NULL);

	if (StrValid(Token)) Args->Port=atoi(Token);
}

}

Destroy(Tempstr);
Destroy(Token);

return(Args);
}




