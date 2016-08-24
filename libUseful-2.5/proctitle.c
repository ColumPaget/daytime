#include "proctitle.h"
#define __GNU_SOURCE
#include "errno.h"

/*This is code to change the command-line of a program as visible in ps */

extern char **environ;
char *TitleBuffer=NULL;
int TitleLen=0;


//The command-line args that we've been passed (argv) will occupy a block of contiguous memory that
//contains these args and the environment strings. In order to change the command-line args we isolate
//this block of memory by iterating through all the strings in it, and making copies of them. The
//pointers in 'argv' and 'environ' are then redirected to these copies. Now we can overwrite the whole
//block of memory with our new command-line arguments.
void ProcessTitleCaptureBuffer(char **argv)
{
char *end=NULL, *tmp;
int i;

TitleBuffer=*argv;
end=*argv;
for (i=0; argv[i] !=NULL; i++)
{
//if memory is contiguous, then 'end' should always wind up
//pointing to the next argv
if (end==argv[i])
{
	while (*end != '\0') end++;
	end++;
}
}

//we used up all argv, environ should follow it
if (argv[i] ==NULL)
{
	for (i=0; environ[i] !=NULL; i++)
	if (end==environ[i])
	{
	while (*end != '\0') end++;
	end++;
	}
}

//now we replace argv and environ with copies
for (i=0; argv[i] != NULL; i++) argv[i]=strdup(argv[i]);
for (i=0; environ[i] != NULL; i++) environ[i]=strdup(environ[i]);

//These might point to argv[0], so make copies of these too
#ifdef __GNU_LIBRARY__
extern char *program_invocation_name;
extern char *program_invocation_short_name;

program_invocation_name=strdup(program_invocation_name);
program_invocation_short_name=strdup(program_invocation_short_name);
#endif


TitleLen=end-TitleBuffer;
}


void ProcessSetTitle(char **argv, char *FmtStr, ...)
{
va_list args;

		if (! TitleBuffer) ProcessTitleCaptureBuffer(argv);
		memset(TitleBuffer,0,TitleLen);

    va_start(args,FmtStr);
		vsnprintf(TitleBuffer,TitleLen,FmtStr,args);
    va_end(args);
}

