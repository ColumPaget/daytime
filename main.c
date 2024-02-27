#include "common.h"
#include "sntp.h"
#include "sysclock.h"
#include "command-line-args.h"


void SetTimeZone()
{

    if (! StrLen(RemoteTimeZone)) RemoteTimeZone=CopyStr(RemoteTimeZone,"");
    StripLeadingWhitespace(RemoteTimeZone);
    StripTrailingWhitespace(RemoteTimeZone);
    xsetenv("TZ",RemoteTimeZone);
}


char *ClipField(char *RetStr, const char *Str, int Pos)
{
    const char *ptr;
    int i;

    ptr=Str;
    for (i=0; i < Pos; i++)
    {
        ptr=GetToken(ptr, " ", &RetStr, 0);
    }

    return(RetStr);
}




time_t ParseDayTimeLine(const char *Line)
{
    time_t val;
    const char *ptr;

    //first try NIST format
    //Modified Julian Day, but we ignore it
    ptr=strchr(Line, ' ');
    while (isspace(*ptr)) ptr++;
    //remainder is time in %y-%m-%d %H:%M:%S
    if ( (StrLen(ptr) > 16) && (ptr[2]=='-'))
    {
        val=DateStrToSecs("%y-%m-%d %H:%M:%S", ptr, RemoteTimeZone);
        if (val > 0) return(val);
    }

    //now try http style format "Mon, 26 Feb 2024 16:06:49 GMT"
    if ((StrLen(Line) > 28) && (Line[3]==','))
    {
        val=DateStrToSecs("%a, %d %b %Y %H:%M:%S", Line, RemoteTimeZone);
        if (val > 0) return(val);
    }

    if ((StrLen(Line) > 18) && (Line[4]=='-'))
    {
        //next try modern iso time format
        val=DateStrToSecs("%Y-%m-%dT%H:%M:%S", Line, RemoteTimeZone);
        if (val > 0) return(val);
    }

    //now try old dayname, monthname, daynum, time, year format
    val=DateStrToSecs("%a %b %d %H:%M:%S %Y", Line, RemoteTimeZone);
    if (val > 0) return(val);


    return(-1);
}



int GetTimeFromDayTimeHost(char *HostName, int Port, struct timeval *Time)
{
    char *Tempstr=NULL, *Line=NULL;
    const char *ptr;
    int count, result=FALSE;
    struct tm TM;
    STREAM *S;

    if (Port==0) Port=13;
    Tempstr=FormatStr(Tempstr, "tcp:%s:%d", HostName, Port);
    S=STREAMOpen(Tempstr, "");
    if (S)
    {
        Line=STREAMReadLine(Line,S);
        while (Line)
        {
            StripTrailingWhitespace(Line);
            if (StrValid(Line)) break;
            Line=STREAMReadLine(Line,S);
        }

        SetTimeZone();
        Time->tv_sec=ParseDayTimeLine(Line);
        if (Time->tv_sec > 0) result=TRUE;

        STREAMClose(S);
    }
    else printf("Failed To Connect to Host %s\n", HostName);

    Destroy(Tempstr);
    Destroy(Line);

    return(result);
}




int GetTimeFromTimeHost(char *HostName, int Port, struct timeval *Time)
{
    uint32_t timeval;
    char *Tempstr=NULL;
    int result=FALSE;
    STREAM *S;

    if (Port==0) Port=37;
    Tempstr=FormatStr(Tempstr, "tcp:%s:%d", HostName, Port);
    S=STREAMOpen(Tempstr, "");
    if (S)
    {
//we read a 32 bit value from the host
        STREAMReadBytes(S,(unsigned char *) &timeval, 4);
        timeval=ntohl(timeval);


//this 32 bit value represents seconds since 1900, we need to convert this to
//seconds since 1970
        timeval-=Time_Server_Offset;
        Time->tv_sec=timeval;
        result=TRUE;
    }
    else
    {
        Tempstr=FormatStr(Tempstr,"Failed To Connect To Time Host %s",HostName);
        //Log("TIME: %s",Tempstr);
    }
    STREAMClose(S);

    Destroy(Tempstr);
    return(result);
}



int GetTimeFromHTTPHost(const char *URL, struct timeval *Time)
{
    char *Tempstr=NULL;
    int result=FALSE;
    STREAM *S;

    S=STREAMOpen(URL, "H");
    if (S)
    {
        Tempstr=CopyStr(Tempstr, STREAMGetValue(S, "HTTP:Date"));
        if (! StrValid(RemoteTimeZone)) RemoteTimeZone=ClipField(RemoteTimeZone, Tempstr, 6);
        Time->tv_sec=DateStrToSecs("%a, %d %b %Y %H:%M:%S", Tempstr, RemoteTimeZone);
        if (Time->tv_sec > 0) result=TRUE;
        STREAMClose(S);
    }

    Destroy(Tempstr);

    return(result);
}



int GetTimeFromSSHHost(const char *URL, struct timeval *Time)
{
    char *Tempstr=NULL, *Line=NULL, *Token=NULL;
    int result=FALSE;
    time_t val;
    STREAM *S;

    Tempstr=MCopyStr(Tempstr, URL, "/date", NULL);
    S=STREAMOpen(Tempstr, "x");
    if (S)
    {
        Line=STREAMReadLine(Line,S);
        while (Line)
        {
            StripTrailingWhitespace(Line);

            if (! StrValid(RemoteTimeZone)) RemoteTimeZone=ClipField(RemoteTimeZone, Line, 5);
            Time->tv_sec=DateStrToSecs("%a %b %d %H:%M:%S %Z %Y", Line, RemoteTimeZone);
            if (Time->tv_sec > 0) //we got a valid time!
            {
                result=TRUE;
                break;
            }

            Line=STREAMReadLine(Line,S);
        }
        STREAMClose(S);
    }

    Destroy(Tempstr);
    Destroy(Token);
    Destroy(Line);
    return(result);
}



int GetTimeFromHost(const char *URL, struct timeval *Time)
{

    int Port=0, result=FALSE;
    char *Proto=NULL, *Host=NULL, *PortStr=NULL;


    ParseURL(URL, &Proto, &Host, &PortStr, NULL, NULL, NULL, NULL);
    Port=atoi(PortStr);
    if (strcmp(Proto, "time")==0) result=GetTimeFromTimeHost(Host,Port,Time);
    else if (strcmp(Proto, "nist")==0) result=GetTimeFromDayTimeHost(Host, Port, Time);
    else if (strcmp(Proto, "daytime")==0) result=GetTimeFromDayTimeHost(Host, Port, Time);
    else if (strcmp(Proto, "http")==0) result=GetTimeFromHTTPHost(URL, Time);
    else if (strcmp(Proto, "https")==0) result=GetTimeFromHTTPHost(URL, Time);
    else if (strcmp(Proto, "ssh")==0) result=GetTimeFromSSHHost(URL, Time);
    else if (strcmp(Proto, "sntp")==0) result=SNTPGetTime(Host, Port, Time);
    else if (strcmp(Proto, "ntp")==0) result=SNTPGetTime(Host, Port, Time);

    Destroy(PortStr);
    Destroy(Proto);
    Destroy(Host);

    return(result);
}



int GoDayTime(TArgs *Args)
{
    struct timeval Time;
    int result=FALSE;

    memset(&Time, 0, sizeof(Time));

//If we've not been told to do anything, then try some default
//behaviors

    if (! StrValid(Args->URL))
    {
        result=GetTimeFromHost("https:www.google.com", &Time);
        if (! result) result=GetTimeFromHost("http:www.google.com", &Time);
        if (! result) result=GetTimeFromHost("ntp:pool.ntp.org", &Time);
    }
    else result=GetTimeFromHost(Args->URL, &Time);

//if we didn't get a command from any timeserver, then don't print output
    if (result == TRUE) HandleReceivedTime(&Time);


    return(result);
}



void SetTimeFromCommandLine(TArgs *Args)
{
    char *Date=NULL, *Time=NULL, *Tempstr=NULL, *Token=NULL, *wptr;
    const char *DateFormats[]= {"%a %b %d %H:%M:%S %Z %Y", "%a %b %d %H:%M:%S %Y", "%b %d %H:%M:%S %Z %Y", "%b %d %H:%M:%S %Y", NULL};
    struct tm TM;
    struct timeval tv;
    const char *ptr;
    int i;


    if (! isdigit(*Args->SetTime))
    {
        tv.tv_usec=0;
        for (i=0; DateFormats[i] != NULL; i++)
        {
            if (strptime(Args->SetTime, DateFormats[i], &TM) != NULL) break;
        }
    }
    else
    {
        //if SetTime contains a 'T' followed by a digit then we have
        //the format 1990/10/18T11:10:00
        ptr=GetToken(Args->SetTime, "\\S|T", &Date, GETTOKEN_MULTI_SEP);
        ptr=GetToken(ptr, "\\S", &Time, 0);

        //if there's a ':' or a '.' in date, then presume date and time are reversed!
        if (strchr(Date, ':') || strchr(Date, '.'))
        {
            Tempstr=CopyStr(Tempstr, Time);
            Time=CopyStr(Time, Date);
            Date=CopyStr(Date, Tempstr);
        }

        strrep(Time, '.', ':');
        //no time? We only have a date, so add curr time on the end
        if (StrLen(Time)==0) Time=CopyStr(Time, GetDateStr("%H:%M:%S",NULL));
        //no seconds?
        else if (StrLen(Time)==5) Time=CatStr(Time, ":00");

        //no date? Create one from today's date
        if (StrLen(Date)==0) Date=CopyStr(Date, GetDateStr("%Y/%m/%d", NULL));
        else
        {
            //we must have a 4-digit year somewhere in date
            if (StrLen(Date)==10)
            {
                ptr=GetToken(Date, "/|-|_|.", &Token, GETTOKEN_MULTI_SEP);
                //we have a 4-digit year at the start of the date
                if (StrLen(Token)==4)
                {
                    Tempstr=MCopyStr(Tempstr, Token, "/", NULL);
                    ptr=GetToken(ptr, "/|-|_|.", &Token, GETTOKEN_MULTI_SEP);
                    Tempstr=MCatStr(Tempstr, Token, "/", NULL);
                    ptr=GetToken(ptr, "/|-|_|.", &Token, GETTOKEN_MULTI_SEP);
                    Tempstr=CatStr(Tempstr, Token);
                }
                //we must have a 4-digit year at the end of the date
                else
                {
                    Tempstr=CopyStrLen(Tempstr, Date+6, 4);
                    Tempstr=CatStr(Tempstr, "/");
                    Tempstr=CatStrLen(Tempstr, Date+3, 2);
                    Tempstr=CatStr(Tempstr, "/");
                    Tempstr=CatStrLen(Tempstr, Date, 2);
                }
            }
            else
            {
                ptr=GetToken(Date, "/|-|_|.", &Token, GETTOKEN_MULTI_SEP);
                Tempstr=MCopyStr(Tempstr, Token, "/", NULL);
                ptr=GetToken(Date, "/|-|_|.", &Token, GETTOKEN_MULTI_SEP);
                Tempstr=MCatStr(Tempstr, Token, "/", NULL);
                ptr=GetToken(Date, "/|-|_|.", &Token, GETTOKEN_MULTI_SEP);
                Tempstr=CatStr(Tempstr, Token);
            }

            Date=CopyStr(Date, Tempstr);
        }

        Tempstr=MCopyStr(Tempstr, Date, " ", Time, NULL);
        printf("Time Parsed As: %s\n",Tempstr);
        strptime(Tempstr, "%Y/%m/%d %H:%M:%S", &TM);
    }


    tv.tv_sec=mktime(&TM);
    tv.tv_usec=0;

    HandleReceivedTime(&tv);


    Destroy(Tempstr);
    Destroy(Token);
    Destroy(Date);
    Destroy(Time);
}


int main(int argc, char *argv[])
{
    char *ptr;
    STREAM *SNTPD=NULL, *S=NULL;
    ListNode *Connections;
    struct timeval tv;
    time_t NextSync=0;
    int result, ExitVal=0;

//just to make sure we always have this
    gettimeofday(&TimeNow, NULL);
    LastError=SetStrLen(LastError, 255);
    Connections=ListCreate();
    ptr=getenv("TZ");
    if (ptr) ptr=strchr(ptr,'=');
    if (ptr) ptr++;
    OldTimeZone=CopyStr(OldTimeZone,ptr);
    if (StrLen(OldTimeZone)==0) OldTimeZone=CopyStr(OldTimeZone,"");

    Args=CommandLineParse(argc, argv);

    if (Args->Flags & FLAG_CMDLINE_TIME) SetTimeFromCommandLine(Args);
    else
    {
        if (Args->Flags & FLAG_SYSLOG) openlog("daytime",LOG_PID,0);
        if (Args->Flags & FLAG_BACKGROUND) demonize();

        if (StrValid(Args->PidFilePath)) WritePidFile(Args->PidFilePath);
        if (Args->Flags & (FLAG_SNTPD | FLAG_BCAST_RECV))
        {
            SNTPD=BindPort("",123);
            if (SNTPD) ListAddItem(Connections, SNTPD);
        }
        srand(getpid() + time(NULL));

        while (1)
        {
            gettimeofday(&TimeNow, NULL);
            if (TimeNow.tv_sec > NextSync)
            {
                if (GoDayTime(Args) == FATAL) break;

                //If we are told to broadcast the time using SNTP, do so. This happens here so that we broadcast
                //the latest time we got from timeservers above, IF we got a time from a timeserver
                //If we didn't get a time, we still run this command to broadcast the system time
                if (Args->Flags & FLAG_BCAST_SEND) result=SNTPBroadcastNets(Args->BcastNets);

                NextSync=TimeNow.tv_sec + Args->SleepTime;
            }
            if (S && (S==SNTPD)) SNTPReceive(S);

            if (! (Args->Flags & FLAG_DEMON)) break;
            tv.tv_sec=Args->SleepTime;
            S=STREAMSelect(Connections, &tv);
        }

        RemoteTimeZone=CopyStr(RemoteTimeZone,OldTimeZone);
        SetTimeZone();
    }

    return(ExitVal);
}
