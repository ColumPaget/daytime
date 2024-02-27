WHAT IS IT
==========
	A program that gets time from daytime, nist daytime, time, SNTP/NTP, ssh, http or https  servers, and optionally updates the system and/or hardware clocks. It can also act as an SNTP server in its own right, or set date and time from the command-line using a number of different date and time formats.

AUTHOR
======
daytime and libUseful are (C) 2010 Colum Paget. They are released under the GPL so you may do anything with them that the GPL allows.

Email: colums.projects@gmail.com
Website: www.cjpaget.co.uk

DISCLAIMER
==========
  This is free software. It comes with no guarentees and I take no responsiblity if it makes your computer explode or opens a portal to the demon dimensions, though if it does the latter I'd like to be notified.

INSTALL
=======
After unpacking the tar-ball:
```
    tar -zxf daytime-0.1.tar.gz
```

Do the usual './configure; make; make install'
```
    cd daytime-1.0
    ./configure
    make
    make install
```


USAGE
=====

```
   daytime -daytime|-time|-nist|-http|-sntp <server> [-s] [-r] [-tz <timezone>]
   daytime <url> [-s] [-r] [-tz <timezone>]
   daytime -sntp-bcast <broadcast net> [-sntp-version <version>]
   daytime -sntpd [-sntp-stratum <stratum>] [-sntp-version <version>]
   daytime -S [date or time specifier]
```


OPTIONS
=======

```
   -daytime	Get time from daytime (RFC-867, port 13) server
   -time		Get time from time (RFC-868, port 123) server
   -nist		Get time from NIST daytime server
   -http		Get time from a web server
   -sntp		Get time from a SNTP server. (Not full NTP, only accurate to seconds). If server arg is 'bcast' then wait to recieve sntp broadcast.
   -ntp 		Get time from a SNTP server. (Not full NTP, only accurate to seconds). If server arg is 'bcast' then wait to recieve sntp broadcast.
   -sntp-bcast	Broadcast SNTP Packets to supplied address. This arg can be used multiple times to bcast to multiple nets on a multihomed host
   -sntp-version	Version of SNTP in packet. Some devices only accept '1' as the version in sntp broadcasts, when it should be '4'.
   -sntp-stratum	Stratum of SNTP in packet. This is a measure of how many 'hops' away from an atomic clock an ntp server is. Defaults to '3'.
   -sntpd	in Daemon mode (implies -d) and provide an SNTP service.
   -d		Daemon mode. Background and stay running. Needed to recieve broadcast times
   -D		Daemon mode WITHOUT BACKGROUNDING.
   -P		Pidfile path for daemon mode.
   -t		Sleep time. Time between checks when in daemon mode, or between SNTP broadcasts (default 30 secs).
   -s		Set clock to time we got from server (requires root permissions).
   -S		Set clock from the command-line (requires root permissions).
   -r		Set hardware RTC clock
   -v		verbose output (mainly for SNTP mode)
   -l		syslog significant events
   -tz		Timezone of remote host.
   -servers	List of some servers to try
   -?		This help
```

The -nist, -http, -sntp and -ntp optioons do not need a server argument, as they fall back to default servers.

If no server specified, http time from www.google.com will be tried first, then ntp from pool.ntp.org.
Servers can be specified as a host/port pair, like 'time.somewhere.com:8080'


Command-line set date/time.
===========================

The '-S' switch allows setting a date or time from the command-line. This can be expressed in one of the following formats:

```
   HH:MM                -  time expressed in hours and minutes, date will stay as current.
   HH:MM:SS             -  time expressed in hours, minutes and seconds, date will stay as current.
   YYYY/mm/dd           -  date expressed in year, month, day. Time will stay as current. 
   dd/mm/YYYY           -  date expressed in year, month, day. Time will stay as current. 
   YYYY/mm/dd HH:MM:SS  -  date and time. 
   dd/mm/YYYY HH:MM:SS  -  date and time. 
   HH:MM:SS YYYY/mm/dd  -  date and time. 
   HH:MM:SS dd/mm/YYYY  -  date and time. 
   YYYY-mm-ddTHH:MM:SS  -  date and time. 
   YYYY/mm/ddTHH:MM:SS  -  date and time. 
   Sun Jan 20 15:55:37 GMT 2019   -  standard output of the 'date' command
   Sun Jan 20 15:55:37 2019       -  'date' style without zone
   Jan 20 15:55:37 GMT 2019       -  'date' style without day
   Jan 20 15:55:37 2019           -  'date' style without day and zone
```

any character can be used as a separator in date, but time needs to use ':'


SNTP Broadcasts and Daemon mode.
================================

Receiving the time as an SNTP broadcast requires having daytime stay running and wait for the message. To faciliate this a 'daemon mode' has been added. When -d or -D is used, daytime will stay running and do whatever it was told to do periodically. So:


```
	daytime -t 600 -d -sntp-bcast 192.168.1.255 -sntp-bcast 192.168.2.255
```


Will send sntp broadcasts of the current time to the networks 192.168.1.x and 192.168.2.x. The -t flag can be used to specify a time between broadcasts.

```
	daytime -s -d -sntp bcast
```

Will persist and wait to recieve sntp broadcasts and set the system time from them. NOTE -t cannot be used in sntp broadcast receive mode

```
	daytime -t 3600 -s -http www.google.com
```

Will check the time with google via http every hour, and set the system time to it


SNTP Server
===========

the -sntpd option will put daytime into SNTP server mode, where it will reply to SNTP requests on port 123. This can be combined with other actions, so for example:

```
	daytime -sntpd -sntp-bcast 192.168.2.255 -daytime time.somewhere.com -t 60
```

Will run as an SNTP server, updating time using daytime protocol to 'time.somewhere.com' every 60 seconds and sending sntp broadcasts every 60 seconds too

Thanks to Robert Crowley (http://tools.99k.org/) and Andrew Benton for bug reports
