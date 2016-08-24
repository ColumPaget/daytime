WHAT IS IT
==========
	A trivial program that gets time from daytime, nist daytime, time, SNTP or http servers, and optionally updates the system and/or hardware clocks. It can also act as an SNTP server in its own right.

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
    tar -zxf daytime-0.1.tar.gz

Do the usual './configure; make; make install'
    cd daytime-1.0
    ./configure
    make
    make install


USAGE
=====

		 daytime  -daytime|-time|-nist|-http|-sntp <server> [-s] [-r] [-tz <timezone>]
		 daytime  -sntp-bcast <broadcast net>
		 -daytime	Get  time from daytime (RFC-867, port 13) server
		 -time		Get  time from time (RFC-868, port 123) server
		 -nist		Get  time from NIST daytime server
		 -http		Get  time from a web server
		 -sntp		Get  time from a SNTP server. (Not full NTP, only accurate to seconds). If server arg is 'bcast' then wait to recieve sntp broadcast.
		 -sntp-bcast	Broadcast  SNTP Packets to supplied address. This arg can be used multiple times to bcast to multiple nets on a multihomed host
		 -sntpd	in  Daemon mode (implies -d) and provide an SNTP service.
		 -d		Daemon  mode. Background and stay running. Needed to recieve broadcast times
		 -D		Daemon  mode WITHOUT BACKGROUNING.
		 -t		Sleep  time. Time between checks when in daemon mode, or between SNTP broadcasts (default 30 secs).
		 -s		Set  clock to time we got from server (requires root permissions).
		 -r		Set  hardware RTC clock
		 -tz		Timezone  of server
		 -servers	List  of some servers to try
		 -?		This  help

If no server specified, http time from www.google.com will be tried first, then nist time time-a.nist.gov then ntp from pool.ntp.org.
Servers can be specified as a host/port pair, like 'time.somewhere.com:8080'

SNTP Broadcasts and Daemon mode.
================================

Receiving the time as an SNTP broadcast requires having daytime stay running and wait for the message. To faciliate this a 'daemon mode' has been added. When -d or -D is used, daytime will stay running and do whatever it was told to do periodically. So:


	daytime -t 600 -d -sntp-bcast 192.168.1.255 -sntp-bcast 192.168.2.255


Will send sntp broadcasts of the current time to the networks 192.168.1.x and 192.168.2.x. The -t flag can be used to specify a time between broadcasts.

	daytime -s -d -sntp bcast

Will persist and wait to recieve sntp broadcasts and set the system time from them. NOTE -t cannot be used in sntp broadcast receive mode

	daytime -t 3600 -s -http www.google.com

Will check the time with google via http every hour, and set the system time to it


SNTP Server
===========

the -sntpd option will put daytime into SNTP server mode, where it will reply to SNTP requests on port 123. This can be combined with other actions, so for example:

	daytime -sntpd -sntp-bcast 192.168.2.255 -daytime time.somewhere.com -t 60

Will run as an SNTP server, updating time using daytime protocol to 'time.somewhere.com' every 60 seconds and sending sntp broadcasts every 60 seconds too

Thanks to Robert Crowley (http://tools.99k.org/) and Andrew Benton for bug reports
