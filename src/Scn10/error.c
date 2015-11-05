/*************************************************************************
*
*  error.c - Error managment stuff.
*
*************************************************************************/

#include "local.h"

/* Client-supplied message output function. */
MSGFN Message = NULL; 

int warning_count;
int error_count;
int fatal_count;

static char tmpbuf[SCN_MSG_MAX];
static char outmsg[SCN_MSG_MAX];

int Error_Init(void)
{
	error_count = warning_count = fatal_count = 0;
	return 1;
}

void Error_Close(void)
{
}

void LogMessage(const char *fmt, ...)
{
	if(Message != NULL)
	{
		va_list ap;
		va_start(ap, fmt);
		vsprintf(outmsg, fmt, ap);
		va_end(ap);
		Message(outmsg);
	}
}

void LogMessageNoFmt(const char *msg)
{
	if(Message != NULL)
		Message(msg);
}

void LogWarning(const char *fmt, ...)
{
	warning_count++;
	if(Message != NULL)
	{
		va_list ap;
		va_start(ap, fmt);
		strcpy(tmpbuf, "Warning: ");
		strcat(tmpbuf, fmt);
		vsprintf(outmsg, tmpbuf, ap);
		va_end(ap);
		Message(outmsg);
	}
}

void LogError(const char *fmt, ...)
{
	error_count++;
	if(Message != NULL)
	{
		va_list ap;
		va_start(ap, fmt);
		strcpy(tmpbuf, "Error: ");
		strcat(tmpbuf, fmt);
		vsprintf(outmsg, tmpbuf, ap);
		va_end(ap);
		Message(outmsg);
	}
}

void LogFatal(const char *fmt, ...)
{
	fatal_count++;
	if(Message != NULL)
	{
		va_list ap;
		va_start(ap, fmt);
		strcpy(tmpbuf, "Fatal: ");
		strcat(tmpbuf, fmt);
		vsprintf(outmsg, tmpbuf, ap);
		va_end(ap);
		Message(outmsg);
	}
}

void LogMemError(const char *who)
{
	if(who != NULL && *who != '\0')
		LogError("%s: Unable to allocate memory.", who);
	else
		LogError("Unable to allocate memory.");
	PrintFileAndLineNumber();
}
