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


void PrintUsage()
{
fprintf(stdout,"\nDaytime: version %s\n",Version);
fprintf(stdout,"Author: Colum Paget\n");
fprintf(stdout,"Email: colums.projects@gmail.com\n");
fprintf(stdout,"Website: www.cjpaget.co.uk\n");
fprintf(stdout,"\n");
printf("Usage:	daytime -daytime|-time|-nist|-http|-sntp <server> [-s] [-r] [-tz <timezone>] [-P <path>]\n");
printf("	daytime -sntp-bcast <broadcast net>\n");
printf("	-daytime	Get time from daytime (RFC-867, port 13) server\n");
printf("	-time		Get time from time (RFC-868, port 123) server\n");
printf("	-nist		Get time from NIST daytime server\n");
printf("	-http		Get time from a web server\n");
printf("	-sntp		Get time from a SNTP server. (Not full NTP, only accurate to seconds). If server arg is 'bcast' then wait to recieve sntp broadcast.\n");
printf("	-sntp-bcast	Broadcast SNTP Packets to supplied address. This arg can be used multiple times to bcast to multiple nets on a multihomed host\n");
printf("	-sntpd	in Daemon mode (implies -d) and provide an SNTP service.\n");
printf("	-d		Daemon mode. Background and stay running. Needed to recieve broadcast times\n");
printf("	-D		Daemon mode WITHOUT BACKGROUNDING.\n");
printf("	-P		Pidfile path for daemon mode.\n");
printf("	-t		Sleep time. Time between checks when in daemon mode, or between SNTP broadcasts (default 30 secs).\n");
printf("	-s		Set clock to time we got from server (requires root permissions).\n");
printf("	-r		Set hardware RTC clock\n");
printf("	-v		verbose output (mainly for SNTP mode)\n");
printf("	-l		syslog significant events\n");
printf("	-tz		Timezone of server\n");
printf("	-servers	List of some servers to try\n");
printf("	-?		This help\n\n");
printf("If no server specified, http time from www.google.com will be tried first, then nist time time-a.nist.gov then ntp from pool.ntp.org.\n");
printf("Servers can be specified as a host/port pair, like 'time.somewhere.com:8080'\n");
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
else if (strcmp(argv[i],"-servers")==0) PrintServers();
else 
{
  if (strcasecmp(argv[i],"bcast")==0)
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
}

DestroyString(Tempstr);
DestroyString(Token);

return(Args);
}




