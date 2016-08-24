#include "common.h"

char *Version="1.0";
char *OldTimeZone=NULL, *CurrTimeZone=NULL;
ListNode *PortBindings=NULL;
struct timeval TimeNow;

STREAM *BindPort(const char *URL, int DefaultPort)
{
	STREAM *S=NULL;
	char *Proto=NULL, *Host=NULL, *Token=NULL, *ptr;
	ListNode *Node;
	int Port, fd=-1;

	if (! PortBindings) PortBindings=ListCreate();
	Node=ListFindNamedItem(PortBindings, URL);
	if (Node) return((STREAM *) Node->Item);

	ParseURL(URL, &Proto, &Host, &Token, NULL, NULL, NULL, NULL);

//	if (strcmp(Proto, "udp")==0)
	{
		if (StrLen(Token))
		{	
			Port=atoi(Token);
			fd=UDPOpen(Host, Port,0);
		}
		if (fd==-1) fd=UDPOpen(Host, DefaultPort,0);
	}

	if (fd > -1)
	{
		S=STREAMFromFD(fd);
		ListAddNamedItem(PortBindings, URL, S);
	}

	DestroyString(Proto);
	DestroyString(Host);
	DestroyString(Token);
	return(S);
}


